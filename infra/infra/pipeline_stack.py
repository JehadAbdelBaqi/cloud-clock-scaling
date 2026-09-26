"""CI/CD: GitHub push -> CodePipeline -> test + synth -> (firmware build) -> deploy.

Uses CDK Pipelines (self-mutating): the pipeline updates itself first when
this file changes, then deploys the app stage.

Needs a CodeStar/CodeConnections connection to GitHub, created once in the
console (Developer Tools -> Settings -> Connections), ARN in cdk.json context.
"""
from aws_cdk import Stack, pipelines
from constructs import Construct

from infra.stage import ClockScaleStage


class PipelineStack(Stack):
    def __init__(
        self,
        scope: Construct,
        construct_id: str,
        *,
        github_repo: str,
        github_branch: str,
        connection_arn: str,
        settings: dict,
        build_firmware: bool = False,
        **kwargs,
    ) -> None:
        super().__init__(scope, construct_id, **kwargs)

        source = pipelines.CodePipelineSource.connection(
            github_repo, github_branch, connection_arn=connection_arn
        )

        # Unit tests gate the synth: a failing test stops the deploy.
        synth = pipelines.ShellStep(
            "Synth",
            input=source,
            commands=[
                "pip install uv",
                "uv sync --locked",
                "uv run pytest -q",
                "cd infra",
                "npx cdk synth",
            ],
            primary_output_directory="infra/cdk.out",
        )

        pipeline = pipelines.CodePipeline(
            self,
            "Pipeline",
            pipeline_name="clock-scale",
            synth=synth,
        )

        pre_steps = []
        if build_firmware:
            # CI for embedded: cross-compile the STM32 firmware on every push.
            # Flashing stays manual — the board is on the desk, not in AWS.
            pre_steps.append(
                pipelines.CodeBuildStep(
                    "FirmwareBuild",
                    input=source,
                    commands=[
                        "pip install platformio",
                        "cd firmware",
                        "pio run",
                    ],
                    primary_output_directory="firmware/.pio/build",
                )
            )

        # Deploy the app into the same account/region as the pipeline.
        pipeline.add_stage(
            ClockScaleStage(self, "Dev", settings=settings, env=kwargs.get("env")),
            pre=pre_steps,
        )
