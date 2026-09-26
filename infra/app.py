#!/usr/bin/env python3
"""CDK entry point.

Settings come from the "context" block in cdk.json (so the pipeline's synth
sees the same values as a local `cdk synth`). Override one-off with
`cdk synth -c key=value`.

Stacks
------
ClockScaleStack          - the app itself; deploy directly with `cdk deploy ClockScaleStack`
ClockScalePipelineStack  - only created once `connectionArn` is set; after the
                           first `cdk deploy ClockScalePipelineStack`, pushes to
                           GitHub deploy everything
"""
import os

import aws_cdk as cdk

from infra.infra_stack import ClockScaleStack
from infra.pipeline_stack import PipelineStack

app = cdk.App()
ctx = app.node.try_get_context


def ctx_int(key: str, default: int) -> int:
    value = ctx(key)
    return default if value in (None, "") else int(value)


env = cdk.Environment(
    account=os.getenv("CDK_DEFAULT_ACCOUNT"),
    region=os.getenv("CDK_DEFAULT_REGION"),
)

settings = {
    "device_id": ctx("deviceId") or "nucleo-01",
    "certificate_arn": ctx("certificateArn") or None,
    "med_threshold": ctx_int("medThreshold", 100),
    "high_threshold": ctx_int("highThreshold", 300),
    "max_level": ctx_int("maxLevel", 1),
    "hold_seconds": ctx_int("holdSeconds", 15),
}

ClockScaleStack(app, "ClockScaleStack", env=env, **settings)

connection_arn = ctx("connectionArn")
if connection_arn:
    PipelineStack(
        app,
        "ClockScalePipelineStack",
        github_repo=ctx("githubRepo"),
        github_branch=ctx("githubBranch") or "master",
        connection_arn=connection_arn,
        settings=settings,
        build_firmware=bool(ctx("buildFirmware")),
        env=env,
    )

app.synth()
