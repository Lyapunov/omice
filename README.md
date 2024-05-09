# omice
Oh, mice

What is O-mice?
- One more minimal chess engine
- Only my minimal chess engine
- An okayish Monte-Carlo chess engine
...

*Currently development in progress, once it's done I'll put a picture here*

Constraints:
 - Minimal sources, one can print them out on a couple A4 pages to store them
   on the shelf.
 - Idiomatic modern C++ only, everything is handmade, not based on libs, not
   importing codes from other people.
 - No tutorial material, no explanations, not holding the hand of bystanders.
 - Plays at the level of a casual chess master.
 - Also plays Fischer random chess (again, like a causal chess master).
 - Can annotate games like a casual chess master. So it doesn't do perfect
   analysis but a simple analysis which is a bit sloppy but still instructive
   and warm for amateur players. The kind of analysis that does not show a
   dozen variations and it is not lamenting over centipawn differences, but
   rather shows the okayish main variation that must work between amateur
   players and yells only if one leaves the right path.
 - No opening theory.
 - No pre-coded theory.
 - Avoiding heuristics as much as it's reasonable, especially if it involves
   hardcoding constants, tables, neural network weights ...
 - Avoid everything that feels like "I know chess and this is a thumb of rule,
   people always tell me so and all the internet says so and it's common 
   knowledge so I'm setting it in the stone inside of my code". Try to go in
   the other way: I'm looking for simple algorithms that give me more than the
   effort I put into them, because the world is like that, if I'm not forceful
   but rather observant and silent and instead of the circulating bullshit I 
   just work on my craft then I start to hear the music of spheres.
 - Algorithms are our profession, engineering is our duty.
 - No code-bloating, no sacrifices for looking good or for imitating the goals
   above are achieved.
 - Each subsequent version must have a shorter and niftier code than the
   previous one, and while it must maintain the overall previous strenght,
   making sacrificies for improvements (like sportsmen do day and night) is
   not a goal. 
 - 30 ms/moves on a recent CPU.
