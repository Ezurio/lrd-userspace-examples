from setuptools import setup, Extension

swclient_module = Extension(
    "swclient",
    include_dirs=["./"],
    libraries=["swupdate-client"],
    sources=["swclient.c"],
)

setup(
    name="swclient",
    version="1.0.0",
    description="swupdate client module written in C",
    ext_modules=[swclient_module],
)
