#!/usr/bin/bash

scriptDir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &> /dev/null && pwd)"
configFile="${1:="${scriptDir}/example-config.json"}"

unbuffer "${scriptDir}/build/main" "${configFile}" 2>&1 | tee "${workspaceFolder}/STREAM-FDL/logs/$(date +'%F-%H-%M').log"
