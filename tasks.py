from invoke import task


def _run(ctx):
    ctx.run("./korova/build/Desktop_Qt_6_9_0-Debug/korova")


@task
def build(ctx, run=False):
    ctx.run("cmake --build korova/build/Desktop_Qt_6_9_0-Debug --target all")

    if run:
        _run(ctx)


@task
def clean(ctx):
    ctx.run("cmake --build korova/build/Desktop_Qt_6_9_0-Debug --target clean")


@task
def run(ctx):
    _run(ctx)
