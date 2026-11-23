#!/bin/bash
# Removes Audiokings Vocals (VST3 + Audio Unit). Double-click to run; it asks for your password.
echo "Removing Audiokings Vocals ..."
sudo rm -rf "/Library/Audio/Plug-Ins/VST3/Audiokings Vocals.vst3" \
            "/Library/Audio/Plug-Ins/Components/Audiokings Vocals.component" \
            "$HOME/Library/Audio/Plug-Ins/VST3/Audiokings Vocals.vst3" \
            "$HOME/Library/Audio/Plug-Ins/Components/Audiokings Vocals.component"
sudo pkgutil --forget com.audiokings.vocals 2>/dev/null
killall -9 AudioComponentRegistrar 2>/dev/null
echo "Done. Your presets are kept in ~/Library/Application Support/Audiokings (delete that folder to remove them too)."
