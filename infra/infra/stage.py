"""Deployable unit used by the pipeline (one Stage = one environment)."""
from aws_cdk import Stage
from constructs import Construct

from infra.infra_stack import ClockScaleStack


class ClockScaleStage(Stage):
    def __init__(self, scope: Construct, construct_id: str, *, settings: dict, **kwargs) -> None:
        super().__init__(scope, construct_id, **kwargs)
        ClockScaleStack(self, "ClockScaleStack", **settings)
