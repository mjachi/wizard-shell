# :wizard: shell

Shell extended from a university project in CS33 @ Brown.
Relatively rudimentary, but decently usable.
Features naive command completions, aliasing, and history, among some other smaller QoL changes.


TODO

 - .wshrc initialization: Have the "stenciling" done for this. Need to simply set up the file read
     and have it parse through in the designated function.
 - Completions: currently, the algorithm for this completion is simply a DFS through the tree, so 
     it relies heavily on alphabetical order, since all of the characters are forced to be lowercase,
     and is therefore not a particularly useful completion, but is mostly an issue when there are an 
     extreme number of binaries on the PATH. e.g. my Mac tends to deliver more useless completions 
     than my Gentoo install because the Gentoo install is just leaner.
     
     I would like to incorporate some sort of priority/ frequency scoring, as well as context.
     e.g. buffer = "cd example_f" --> consider only directory names --> "cd example_folder"
     and (Gentoo specific command) "em" should --> "emerge" rather than "emacs", since we 
     don't want to open another operating system
 - Coloring: going to make the splash and prompt look a bit nicer. TERM colors are not the most
     vibrant/ conducive to a good graphic design, however. Might not happen.
