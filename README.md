###### code by claude

- icon
- android

- show screencast of amberol-qt and replicate it
- new alt UI similar to amberol-qt with smooth transitions,keyboard shortcut, and maybe support for visualizer
- blur optimize memory; not releasing memory / constant memory
- need good animation
- blur for songs with no cover art
- maybe add loop track and directory? try to directly place the original loop/shuffle buttons instead of duplicating code, do not directly modify the new UI
- library-music-symbolic icon is blurred?

- size...
- normal window cover art too small?
- playlist width is too small (see workout playlist) [fixed?]

---

- new loop/shuffle: it works mostly, pretty great. it needs a context menu like the main UI's button; update the playlist with the shuffle queue if shuffled, with title as 'queue'.
- playlist bug: yeah, still doesn't work. here's the current snippet:
- fallback: might there be any dimension limits or something? the fallback image is a desktop wallpaper and not a cover art. does it matter? anyway, image exists in the path, and here's the qrc file:
- also, fallback icons in some places appear to be blurry, mainly in the playlist view. any fixed? see pic.

---

- contxt menu: full code + location
- playlist bug: nope, still isn't working. do note that toggling the view off and back on does fix it. but yeah, i want it it not leave that weird artifact.
