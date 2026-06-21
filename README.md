# warning
bleach in your eyes will not undo reading the code in 
- UnleashedRecomp/ui/tas_windows.cpp
- UnleashedRecomp/ui/tas_hooks.h
- UnleashedRecomp/ui/tas_json.h
- UnleashedRecomp/ui/speedometer.h
- whatever else shows up in git diff

# need to do
- fix the position load to respect camera perspectives
- quickboot
- more speedometer options

# i will never do
- medals minimap
- full gameobject semi-savestate loader
- m speed indicator (i seriously can't even find this "airdrag" value)
- menu to change levels without going back to the globe world

# if sdl doesn't compile (linux specific I think)
[Fix](https://github.com/libsdl-org/SDL/commit/6be87ceb33a9aad3bf5204bb13b3a5e8b498fd26)
cast 
node->proxy with (struct pw_node*)
