# Monocle
## A Minimal, Native Console-Like library based on SDL ==

Monocle is a platform for writing 2D sprite-based games that is designed to have as simple as possible of an API while still enabling works of great sophistication. It draws inspiration from YoYoGames's _Game Maker+ systems, as well as from the core engines behind the game projects I've worked on over the years (predominantly _The Ur-Quan Masters_ and _Sable_).

The Monocle Library is built in layers. Each layer is designed to operate with a well-defined C interface, to make embedding parts of it in other languages as straightforward as possible. If a layer does not mesh well with the target language, a cut may be made at a lower level.

Currently only layers 0 and 1 have a full design. The API as the design evolves may be found in the doc/ subdirectory.
