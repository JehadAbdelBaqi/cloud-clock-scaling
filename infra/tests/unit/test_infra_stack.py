"""Synth-level tests: assert on the CloudFormation template. No AWS calls."""
import aws_cdk as cdk
import aws_cdk.assertions as assertions
import pytest

from infra.infra_stack import ClockScaleStack

CERT_ARN = "arn:aws:iot:eu-west-2:123456789012:cert/abc123"
ENDPOINT = "abc123-ats.iot.eu-west-2.amazonaws.com"


def synth(**kwargs) -> assertions.Template:
    app = cdk.App()
    stack = ClockScaleStack(app, "TestStack", iot_endpoint=ENDPOINT, **kwargs)
    return assertions.Template.from_stack(stack)


@pytest.fixture(scope="module")
def template():
    return synth()


def test_thing_named_after_device(template):
    template.has_resource_properties("AWS::IoT::Thing", {"ThingName": "nucleo-01"})


def test_rule_routes_readings_to_lambda(template):
    template.has_resource_properties(
        "AWS::IoT::TopicRule",
        {
            "TopicRulePayload": {
                "Sql": "SELECT *, topic(2) AS device_id FROM 'clockscale/+/readings'",
                "AwsIotSqlVersion": "2016-03-23",
                "Actions": [{"Lambda": assertions.Match.any_value()}],
            }
        },
    )


def test_lambda_runtime_and_env(template):
    template.has_resource_properties(
        "AWS::Lambda::Function",
        {
            "Handler": "handler.handler",
            "Runtime": "python3.12",
            "Environment": {
                "Variables": assertions.Match.object_like(
                    {
                        "MAX_LEVEL": "1",
                        "TOPIC_ROOT": "clockscale",
                        "IOT_ENDPOINT": ENDPOINT,
                    }
                )
            },
        },
    )


def test_iot_can_invoke_lambda(template):
    template.has_resource_properties(
        "AWS::Lambda::Permission",
        {"Action": "lambda:InvokeFunction", "Principal": "iot.amazonaws.com"},
    )


def test_decide_log_group_one_week(template):
    template.has_resource_properties("AWS::Logs::LogGroup", {"RetentionInDays": 7})


def test_log_group_deleted_with_stack(template):
    # No orphaned log groups left behind after `cdk destroy`.
    log_groups = template.find_resources("AWS::Logs::LogGroup")
    assert len(log_groups) == 1
    lg = next(iter(log_groups.values()))
    assert lg["DeletionPolicy"] == "Delete"


def test_device_policy_has_four_statements(template):
    policies = template.find_resources("AWS::IoT::Policy")
    assert len(policies) == 1
    doc = next(iter(policies.values()))["Properties"]["PolicyDocument"]
    actions = sorted(s["Action"] for s in doc["Statement"])
    assert actions == ["iot:Connect", "iot:Publish", "iot:Receive", "iot:Subscribe"]


def test_no_cert_attachments_without_cert(template):
    template.resource_count_is("AWS::IoT::PolicyPrincipalAttachment", 0)
    template.resource_count_is("AWS::IoT::ThingPrincipalAttachment", 0)


def test_cert_attachments_when_cert_given():
    t = synth(certificate_arn=CERT_ARN)
    t.has_resource_properties("AWS::IoT::PolicyPrincipalAttachment", {"Principal": CERT_ARN})
    t.has_resource_properties("AWS::IoT::ThingPrincipalAttachment", {"Principal": CERT_ARN})
