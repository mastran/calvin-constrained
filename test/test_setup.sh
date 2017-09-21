#!/bin/bash

pushd ../
git clone https://github.com/PatrikAAberg/dmce.git
popd

# Init submodules
git submodule init
git submodule update

# fetch calvin-base and install required packages
git clone https://github.com/EricssonResearch/calvin-base.git
pushd calvin-base
git fetch
git checkout d3f48219bc2eb078810f3085643998a1a5c635e3
pip install -r requirements.txt
pip install -r test-requirements.txt
popd
