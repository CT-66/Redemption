###### code by claude

- icon
- android

- search not resetting loop
- show screencast of amberol-qt and replicate it
- new alt UI similar to amberol-qt with smooth transitions,keyboard shortcut, and maybe support for visualizer
- blur optimize memory; not releasing memory / constant memory
- need good animation
- blur for songs with no cover art
- maybe add loop track and directory? try to directly place the original loop/shuffle buttons instead of duplicating code, do not directly modify the new UI
- library-music-symbolic icon is blurred?

- size...
- playlist width is too small (see workout playlist) [fixed?]

--

- also, can we fix the cover art size thing. right now, it resizes on maximize, but i want the older size for normal windows, and a smaller size with position shifted a bit higher for the maximized one.
- one more, an unrelated bug. remember that bug where looped / shuffled songs did not clear their loop / shuffle if the current track was changed using search? i tried the snippet and it does not work. for reference, here's the snippet you gave:

--

- great, the search bug is fixed - but only for loop. i want the same reset for shuffle too.
- cover: yeah, it's fixed. but i want it to be positioned a bit higher, along with the labels and buttons. see pics for the current and intended layouts. note that there's a blank space between the buttons and volume bar, that's because i want a loop and shuffle button in that row. more on this later.
- new loop and shuffle: this is loop and shuffle for the playlist UI. in order to avoid duplication of code and bugs and what not, is it possible to simply "copy" the loop and shuffle buttons from the main window with their functionality. by copy, i mean in a way make them act exactly as if the bottom (main UI) buttons were clicked. for shuffle, when shuffle it toggled, the playlist ought to be replaced with the queue (and update once the queue is over). in short, i simply want to "copy" the existing loop/shuffle buttons without re implementing their functionality and everything again, with a simple addition for the queue. is it possible?

- fallback blur: mind posting all the changes? i'm having trouble.
