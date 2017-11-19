cd shaders

%VK_SDK_PATH%\Bin\glslangValidator.exe -V graphics.vert
move vert.spv graphics.vert.spv
%VK_SDK_PATH%\Bin\glslangValidator.exe -V graphics.frag
move frag.spv graphics.frag.spv

%VK_SDK_PATH%\Bin\glslangValidator.exe -V grass.vert
move vert.spv grass.vert.spv
%VK_SDK_PATH%\Bin\glslangValidator.exe -V grass.frag
move frag.spv grass.frag.spv
%VK_SDK_PATH%\Bin\glslangValidator.exe -V grass.tesc
move tesc.spv grass.tesc.spv
%VK_SDK_PATH%\Bin\glslangValidator.exe -V grass.tese
move tese.spv grass.tese.spv

%VK_SDK_PATH%\Bin\glslangValidator.exe -V compute.comp
move comp.spv compute.comp.spv

