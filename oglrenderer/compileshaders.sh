./shaderc/glslc.exe ./shaders/quad.vert -o ./spv/vert.spv --target-env=opengl -std=450core -I "./" -I "./shaders"
./shaderc/glslc.exe ./shaders/quad.frag -o ./spv/frag.spv --target-env=opengl -std=450core -I "./" -I "./shaders"
./shaderc/glslc.exe ./shaders/texturedQuad.frag -o ./spv/texturedQuadFrag.spv --target-env=opengl -std=450core -I "./" -I "./shaders"
./shaderc/glslc.exe ./shaders/fbmnoise.frag -o ./spv/fbmnoisefrag.spv --target-env=opengl -std=450core -I "./" -I "./shaders"
./shaderc/glslc.exe ./shaders/perlinnoise.frag -o ./spv/perlinnoisefrag.spv --target-env=opengl -std=450core -I "./" -I "./shaders"
./shaderc/glslc.exe ./shaders/worleynoise.frag -o ./spv/worleynoisefrag.spv --target-env=opengl -std=450core -I "./" -I "./shaders"
./shaderc/glslc.exe ./shaders/cloudnoise.frag -o ./spv/cloudnoisefrag.spv --target-env=opengl -std=450core -I "./" -I "./shaders"
./shaderc/glslc.exe ./shaders/perlinworleynoise.frag -o ./spv/perlinworleynoisefrag.spv --target-env=opengl -std=450core -I "./" -I "./shaders"
./shaderc/glslc.exe ./shaders/precomputecloud.comp -o ./spv/precomputecloud.spv --target-env=opengl -std=450core -I "./" -I "./shaders"
./shaderc/glslc.exe ./shaders/precomputeenvironment.frag -o ./spv/precomputeenvironment.spv --target-env=opengl -std=450core -I "./" -I "./shaders"
./shaderc/glslc.exe ./shaders/precomputesky.frag -o ./spv/precomputesky.spv --target-env=opengl -std=450core -I "./" -I "./shaders"
./shaderc/glslc.exe ./shaders/oceanheightfield.comp -o ./spv/oceanheightfield.spv --target-env=opengl -std=450core -I "./" -I "./shaders"
./shaderc/glslc.exe ./shaders/oceanhfinal.comp -o ./spv/oceanhfinal.spv --target-env=opengl -std=450core -I "./" -I "./shaders"
./shaderc/glslc.exe ./shaders/precomputebutterfly.comp -o ./spv/precomputebutterfly.spv --target-env=opengl -std=450core -I "./" -I "./shaders"
./shaderc/glslc.exe ./shaders/butterflyoperation.comp -o ./spv/butterflyoperation.spv --target-env=opengl -std=450core -I "./" -I "./shaders"
./shaderc/glslc.exe ./shaders/inversion.comp -o ./spv/inversion.spv --target-env=opengl -std=450core -I "./" -I "./shaders"
./shaderc/glslc.exe ./shaders/water.vert -o ./spv/watervert.spv --target-env=opengl -std=450core -I "./" -I "./shaders"
./shaderc/glslc.exe ./shaders/water.frag -o ./spv/waterfrag.spv --target-env=opengl -std=450core -I "./" -I "./shaders"