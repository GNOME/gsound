
gsound_play_sources = files(
  'gsound-play.vala'
)

gsound_play_dependencies = [
  gio,
  gsound_vapi
]

executable(
  'gsound-play',
  gsound_play_sources,
  dependencies: gsound_play_dependencies,
  install: true
)
