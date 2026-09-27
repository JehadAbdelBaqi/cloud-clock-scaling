#!/usr/bin/env python3
"""CDK entry point.

Settings come from the "context" block in cdk.json. Override one-off with
`cdk synth -c key=value`.

Stacks
------
ClockScaleStack  - the app itself; deploy with `cdk deploy ClockScaleStack`
"""
import os

import aws_cdk as cdk

from infra.infra_stack import ClockScaleStack

app = cdk.App()
ctx = app.node.try_get_context


def ctx_int(key: str, default: int) -> int:
    value = ctx(key)
    return default if value in (None, "") else int(value)


env = cdk.Environment(
    account=os.getenv("CDK_DEFAULT_ACCOUNT"),
    region=os.getenv("CDK_DEFAULT_REGION"),
)

iot_endpoint = ctx("iotEndpoint")
if not iot_endpoint:
    raise SystemExit(
        "Set \"iotEndpoint\" in infra/cdk.json -> context. Get it with:\n"
        "  aws iot describe-endpoint --endpoint-type iot:Data-ATS"
    )

settings = {
    "iot_endpoint": iot_endpoint,
    "device_id": ctx("deviceId") or "nucleo-01",
    "certificate_arn": ctx("certificateArn") or None,
    "med_threshold": ctx_int("medThreshold", 100),
    "high_threshold": ctx_int("highThreshold", 300),
    "max_level": ctx_int("maxLevel", 1),
    "hold_seconds": ctx_int("holdSeconds", 15),
}

ClockScaleStack(app, "ClockScaleStack", env=env, **settings)

app.synth()
