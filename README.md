# First-assignment-2D-game
'Cursed nightmare' - 2d labyrinth escape game with RPG elements

Link to the Youtube demo: https://www.youtube.com/watch?v=W2D8fktYpAs

This is the game, designed for the "Intro to Programming" assignment at Goldsmiths University. The game is using Octet framework, based on the OpenGL, which is written by Andy Thomasson.

The idea of the game is the story of a boy, who wokes up in the labyrinth and is trying to find the way out. During his journey he feels that with every move he makes, he looses a part of his soul (energy), but fortunately sometimes he finds some soul shards, which refills his reserves. Going from level to level he is starting to undestand, that the soul shards are the essense of the previous victims, because he is hearing their voices on pickup and that such long maze can not be real, which leads to the idea, that he is still dreaming. But he can't wake up, so the only conclusion is that something or someone is holding him.

The initial idea - is fixed level story game with more text, art and objects, but it was cut because of the lack of time and unimportancy of the complexity of this project.

The source code files, that were designed for this application are: 
1. labyrinth_app.h 
  This is the core file of the game, which contains class, inherited from the app class, which handles initialization of the game and game loop (simulate function). Also it handles the drawing of the game levels, based on the data, received from the Labyrinth class.
2. labyrinth.h
  This file handles creation procedurally generated labyrinth, based on the 'Recursive backtracker algorithm' (https://en.wikipedia.org/wiki/Maze_generation_algorithm).
3. camera.h
  The game has dynamical camera, which follows the player and zooms in and out, depending on the current energy level of the character. The class, responsible for it (Camera) is in this file.
4. game_level.h
 This file, contains enumerations and storage classes for game levels.
 
Every level is created using the same random-based recursive algorithm. The pickups are put to the furthest tunnels in the labyrinth (deadends). The goal is to get to the exit, which is represented by staircase. However, each level contains 3 staircases, two of which are fake. Fake staircases - are evil ghosts, and sometimes they are blnking (showing their real nature), when you are making a step, and the lower your energy level is - the bigger chance that they would reveal themself for a moment, unable to handle the neverending hunger.

When the character makes a step - he looses energy, and the walls are getting closer, which is shown by zooming in camera.

The pickups can be 2 types : bonus energy, or double energy. The amount of bonus is 25% and it's absolute value is equal to the amount of energy you loose, when you rush into the ghost.

If you have completed the level - then all you remaining energy would be stored for the next level in 'Reserve'. If you are out of energy - then you reserves come along, which is similar to 'lifes' mechanic in adventure games.
