Import("env")

env.AddCustomTarget(
    name="preact_build",
    dependencies=None,
    actions=[
        "cd preact && npm run build",
        "pio run"
    ],
    title="Preact Build",
    description="Build Preact site and build project with PlatformIO"
)