"""Main stack: everything the device talks to in AWS.

Resources
---------
- IoT Thing + IoT policy (device identity / what it may publish+subscribe)
- Optional cert attachments (cert is created outside CDK — see scripts/)
- Lambda: picks a clock level from a sound reading, publishes the command,
  logs one JSON line per reading to its log group (the history)
- IoT topic rule: readings topic -> Lambda
- Custom resource: looks up this account's IoT data (ATS) endpoint
"""
import os

from aws_cdk import (
    Aws,
    CfnOutput,
    Duration,
    RemovalPolicy,
    Stack,
    aws_iam as iam,
    aws_iot as iot,
    aws_lambda as _lambda,
    aws_logs as logs,
    custom_resources as cr,
)
from constructs import Construct

LAMBDA_SRC = os.path.join(os.path.dirname(__file__), "..", "..", "lambda")

TOPIC_ROOT = "clockscale"


class ClockScaleStack(Stack):
    def __init__(
        self,
        scope: Construct,
        construct_id: str,
        *,
        device_id: str = "nucleo-01",
        certificate_arn: str | None = None,
        med_threshold: int = 100,
        high_threshold: int = 300,
        max_level: int = 1,
        hold_seconds: int = 15,
        **kwargs,
    ) -> None:
        super().__init__(scope, construct_id, **kwargs)

        readings_topic = f"{TOPIC_ROOT}/{device_id}/readings"
        status_topic = f"{TOPIC_ROOT}/{device_id}/status"
        commands_topic = f"{TOPIC_ROOT}/{device_id}/commands"

        def iot_arn(resource: str) -> str:
            return f"arn:{Aws.PARTITION}:iot:{Aws.REGION}:{Aws.ACCOUNT_ID}:{resource}"

        # ---- Device identity -------------------------------------------------
        thing = iot.CfnThing(self, "Thing", thing_name=device_id)

        # Least privilege: this device may only connect as itself, publish its
        # own readings/status, and receive its own commands.
        device_policy = iot.CfnPolicy(
            self,
            "DevicePolicy",
            policy_name=f"{TOPIC_ROOT}-{device_id}",
            policy_document={
                "Version": "2012-10-17",
                "Statement": [
                    {
                        "Effect": "Allow",
                        "Action": "iot:Connect",
                        "Resource": iot_arn(f"client/{device_id}"),
                    },
                    {
                        "Effect": "Allow",
                        "Action": "iot:Publish",
                        "Resource": [
                            iot_arn(f"topic/{readings_topic}"),
                            iot_arn(f"topic/{status_topic}"),
                        ],
                    },
                    {
                        "Effect": "Allow",
                        "Action": "iot:Subscribe",
                        "Resource": iot_arn(f"topicfilter/{commands_topic}"),
                    },
                    {
                        "Effect": "Allow",
                        "Action": "iot:Receive",
                        "Resource": iot_arn(f"topic/{commands_topic}"),
                    },
                ],
            },
        )

        # The certificate is made outside CDK (scripts/create-device-cert.sh),
        # because CloudFormation can't hand back a private key. Once its ARN is
        # in cdk.json context, CDK attaches the policy + thing to it.
        if certificate_arn:
            policy_attach = iot.CfnPolicyPrincipalAttachment(
                self,
                "PolicyCertAttachment",
                policy_name=device_policy.ref,
                principal=certificate_arn,
            )
            policy_attach.add_dependency(device_policy)

            thing_attach = iot.CfnThingPrincipalAttachment(
                self,
                "ThingCertAttachment",
                thing_name=thing.ref,
                principal=certificate_arn,
            )
            thing_attach.add_dependency(thing)

        # ---- IoT data endpoint lookup ---------------------------------------
        # The Lambda publishes commands through the account's ATS data
        # endpoint. It's account-specific, so look it up at deploy time.
        # Own log group so it's deleted with the stack — CDK's default one for
        # the helper Lambda is retained and piles up across deploy/destroy.
        endpoint_lookup_logs = logs.LogGroup(
            self,
            "IotEndpointLookupLogs",
            retention=logs.RetentionDays.ONE_WEEK,
            removal_policy=RemovalPolicy.DESTROY,
        )
        endpoint_lookup = cr.AwsCustomResource(
            self,
            "IotEndpointLookup",
            log_group=endpoint_lookup_logs,
            on_create=cr.AwsSdkCall(
                service="Iot",
                action="describeEndpoint",
                parameters={"endpointType": "iot:Data-ATS"},
                physical_resource_id=cr.PhysicalResourceId.of("IotDataAtsEndpoint"),
            ),
            on_update=cr.AwsSdkCall(
                service="Iot",
                action="describeEndpoint",
                parameters={"endpointType": "iot:Data-ATS"},
                physical_resource_id=cr.PhysicalResourceId.of("IotDataAtsEndpoint"),
            ),
            policy=cr.AwsCustomResourcePolicy.from_sdk_calls(
                resources=cr.AwsCustomResourcePolicy.ANY_RESOURCE
            ),
        )
        iot_endpoint = endpoint_lookup.get_response_field("endpointAddress")

        # ---- Decision Lambda -------------------------------------------------
        # The log group is the reading history: one JSON line per reading,
        # queryable in CloudWatch Logs Insights.
        decide_logs = logs.LogGroup(
            self,
            "DecideClockFnLogs",
            retention=logs.RetentionDays.ONE_WEEK,
            removal_policy=RemovalPolicy.DESTROY,
        )

        decide_fn = _lambda.Function(
            self,
            "DecideClockFn",
            runtime=_lambda.Runtime.PYTHON_3_12,
            handler="handler.handler",
            code=_lambda.Code.from_asset(
                LAMBDA_SRC,
                exclude=["tests", "tests/*", "__pycache__", "*.pyc", "requirements-dev.txt"],
            ),
            memory_size=128,
            timeout=Duration.seconds(5),
            log_group=decide_logs,
            environment={
                "IOT_ENDPOINT": iot_endpoint,
                "TOPIC_ROOT": TOPIC_ROOT,
                "MED_THRESHOLD": str(med_threshold),
                "HIGH_THRESHOLD": str(high_threshold),
                "MAX_LEVEL": str(max_level),
                "HOLD_SECONDS": str(hold_seconds),
            },
        )

        decide_fn.add_to_role_policy(
            iam.PolicyStatement(
                actions=["iot:Publish"],
                resources=[iot_arn(f"topic/{TOPIC_ROOT}/*/commands")],
            )
        )

        # ---- Topic rule: readings -> Lambda ---------------------------------
        # topic(2) pulls the device id out of clockscale/<device_id>/readings.
        rule = iot.CfnTopicRule(
            self,
            "ReadingsRule",
            topic_rule_payload=iot.CfnTopicRule.TopicRulePayloadProperty(
                sql=f"SELECT *, topic(2) AS device_id FROM '{TOPIC_ROOT}/+/readings'",
                aws_iot_sql_version="2016-03-23",
                rule_disabled=False,
                actions=[
                    iot.CfnTopicRule.ActionProperty(
                        lambda_=iot.CfnTopicRule.LambdaActionProperty(
                            function_arn=decide_fn.function_arn
                        )
                    )
                ],
            ),
        )

        decide_fn.add_permission(
            "AllowIotRuleInvoke",
            principal=iam.ServicePrincipal("iot.amazonaws.com"),
            source_arn=rule.attr_arn,
            source_account=Aws.ACCOUNT_ID,
        )

        # ---- Outputs ---------------------------------------------------------
        CfnOutput(self, "IotEndpoint", value=iot_endpoint)
        CfnOutput(self, "ThingName", value=thing.ref)
        CfnOutput(self, "DevicePolicyName", value=device_policy.ref)
        CfnOutput(self, "ReadingsTopic", value=readings_topic)
        CfnOutput(self, "CommandsTopic", value=commands_topic)
        CfnOutput(self, "DecideFunctionName", value=decide_fn.function_name)
        CfnOutput(self, "DecideLogGroup", value=decide_logs.log_group_name)
