#!/usr/bin/env bash
set -euo pipefail

removed=0
remove_user_path() {
  local path="$1"
  if [[ -e "$path" ]]; then
    echo "Removing user plug-in: $path"
    rm -rf "$path"
    removed=1
  fi
}
remove_system_path() {
  local path="$1"
  if [[ -e "$path" ]]; then
    echo "Removing system plug-in (administrator permission may be requested): $path"
    sudo rm -rf "$path"
    removed=1
  fi
}

remove_user_path "$HOME/Library/Audio/Plug-Ins/VST3/FUTURO Spring Reverb.vst3"
remove_user_path "$HOME/Library/Audio/Plug-Ins/Components/FUTURO Spring Reverb.component"
remove_system_path "/Library/Audio/Plug-Ins/VST3/FUTURO Spring Reverb.vst3"
remove_system_path "/Library/Audio/Plug-Ins/Components/FUTURO Spring Reverb.component"

if [[ "$removed" == "0" ]]; then
  echo "No installed FUTURO plug-in bundle was found."
else
  echo "FUTURO bundles removed. Restart the DAW and rescan plug-ins."
fi
