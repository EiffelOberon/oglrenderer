./shaderc/glslc.exe ./shaders/quad.vert -o ./spv/vert.spv --target-env=opengl -std=450core -I "./" -I "./shaders"
./shaderc/glslc.exe ./shaders/quad.frag -o ./spv/frag.spv --target-env=opengl -std=450core -I "./" -I "./shaders"
./shaderc/glslc.exe ./shaders/texturedQuad.frag -o ./spv/texturedQuadFrag.spv --target-env=opengl -std=450core -I "./" -I "./shaders"