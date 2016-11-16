/////////////////////////////////////////////////////////////  
///////////////////CS430 Project 4////////////////////////  
/////////////////////Frankie Berry/////////////////////////  
////////////////November 15, 2016//////////////////////  
////////////////////////////////////////////////////////////

 This application is intended to take in a desired width and height as well as a .json scene input file and .ppm output filename and properly write it out to the designated .ppm file; in contrast with the previous project, this application handles reflection and refraction when interpreting the scene.

 There are no special notes regarding the usage of the program; the program can be built from the makefile and commands should be sent
according to the usage pattern given in the project criteria. The program assumes that a properly formatted json input file
should be passed to the command line and that it should also exist, but there is error checking in the case that it doesn't. Also, if
the output file specified does not exist, then one will be created. 

 Note: However, I wanted to mention that as the program is currently, it seems only somewhat successful at implementing reflections/refractions. I would like to fix this at a later date but I just wanted to mention that I'm not entirely sure how much of each aspect (reflection/refraction) was implemented successfully as I wasn't able to compare against a verified example. If possible I'd really like to get some feedback on where my logic went wrong in the program.
