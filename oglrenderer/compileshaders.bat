%cd%/shaderc/glslc.exe %cd%/shaders/quad.vert -o %cd%/spv/vert.spv --target-env=opengl -std=450core -I "%cd%/" -I "%cd%/shaders"
%cd%/shaderc/glslc.exe %cd%/shaders/quad.frag -o %cd%/spv/frag.spv --target-env=opengl -std=450core -I "%cd%/" -I "%cd%/shaders"
%cd%/shaderc/glslc.exe %cd%/shaders/texturedQuad.frag -o %cd%/spv/texturedQuadFrag.spv --target-env=opengl -std=450core -I "%cd%/" -I "%cd%/shaders"
%cd%/shaderc/glslc.exe %cd%/shaders/fbmnoise.frag -o %cd%/spv/fbmnoisefrag.spv --target-env=opengl -std=450core -I "%cd%/" -I "%cd%/shaders"
%cd%/shaderc/glslc.exe %cd%/shaders/perlinnoise.frag -o %cd%/spv/perlinnoisefrag.spv --target-env=opengl -std=450core -I "%cd%/" -I "%cd%/shaders"
%cd%/shaderc/glslc.exe %cd%/shaders/worleynoise.frag -o %cd%/spv/worleynoisefrag.spv --target-env=opengl -std=450core -I "%cd%/" -I "%cd%/shaders"
%cd%/shaderc/glslc.exe %cd%/shaders/cloudnoise.frag -o %cd%/spv/cloudnoisefrag.spv --target-env=opengl -std=450core -I "%cd%/" -I "%cd%/shaders"
%cd%/shaderc/glslc.exe %cd%/shaders/perlinworleynoise.frag -o %cd%/spv/perlinworleynoisefrag.spv --target-env=opengl -std=450core -I "%cd%/" -I "%cd%/shaders"
%cd%/shaderc/glslc.exe %cd%/shaders/precomputecloud.comp -o %cd%/spv/precomputecloud.spv --target-env=opengl -std=450core -I "%cd%/" -I "%cd%/shaders"
%cd%/shaderc/glslc.exe %cd%/shaders/precomputeenvironment.frag -o %cd%/spv/precomputeenvironment.spv --target-env=opengl -std=450core -I "%cd%/" -I "%cd%/shaders"
%cd%/shaderc/glslc.exe %cd%/shaders/precomputesky.frag -o %cd%/spv/precomputesky.spv --target-env=opengl -std=450core -I "%cd%/" -I "%cd%/shaders"
%cd%/shaderc/glslc.exe %cd%/shaders/oceanheightfield.comp -o %cd%/spv/oceanheightfield.spv --target-env=opengl -std=450core -I "%cd%/" -I "%cd%/shaders"
%cd%/shaderc/glslc.exe %cd%/shaders/oceanhfinal.comp -o %cd%/spv/oceanhfinal.spv --target-env=opengl -std=450core -I "%cd%/" -I "%cd%/shaders"
%cd%/shaderc/glslc.exe %cd%/shaders/precomputebutterfly.comp -o %cd%/spv/precomputebutterfly.spv --target-env=opengl -std=450core -I "%cd%/" -I "%cd%/shaders"
%cd%/shaderc/glslc.exe %cd%/shaders/butterflyoperation.comp -o %cd%/spv/butterflyoperation.spv --target-env=opengl -std=450core -I "%cd%/" -I "%cd%/shaders"
%cd%/shaderc/glslc.exe %cd%/shaders/inversion.comp -o %cd%/spv/inversion.spv --target-env=opengl -std=450core -I "%cd%/" -I "%cd%/shaders"
%cd%/shaderc/glslc.exe %cd%/shaders/water.vert -o %cd%/spv/watervert.spv --target-env=opengl -std=450core -I "%cd%/" -I "%cd%/shaders"
%cd%/shaderc/glslc.exe %cd%/shaders/water.frag -o %cd%/spv/waterfrag.spv --target-env=opengl -std=450core -I "%cd%/" -I "%cd%/shaders"
%cd%/shaderc/glslc.exe %cd%/shaders/temporalquad.vert -o %cd%/spv/temporalvert.spv --target-env=opengl -std=450core -I "%cd%/" -I "%cd%/shaders"
%cd%/shaderc/glslc.exe %cd%/shaders/temporalquad.frag -o %cd%/spv/temporalfrag.spv --target-env=opengl -std=450core -I "%cd%/" -I "%cd%/shaders"
%cd%/shaderc/glslc.exe %cd%/shaders/precomputefresnel.comp -o %cd%/spv/precomputefresnel.spv --target-env=opengl -std=450core -I "%cd%/" -I "%cd%/shaders"
%cd%/shaderc/glslc.exe %cd%/shaders/precomputeirradiance.frag -o %cd%/spv/precomputeirradiancefrag.spv --target-env=opengl -std=450core -I "%cd%/" -I "%cd%/shaders"
%cd%/shaderc/glslc.exe %cd%/shaders/sceneobject.vert -o %cd%/spv/sceneobjvert.spv --target-env=opengl -std=450core -I "%cd%/" -I "%cd%/shaders"
%cd%/shaderc/glslc.exe %cd%/shaders/sceneobject.frag -o %cd%/spv/sceneobjfrag.spv --target-env=opengl -std=450core -I "%cd%/" -I "%cd%/shaders"
%cd%/shaderc/glslc.exe %cd%/shaders/prefilterenvironment.frag -o %cd%/spv/prefilterenvironmentfrag.spv --target-env=opengl -std=450core -I "%cd%/" -I "%cd%/shaders"