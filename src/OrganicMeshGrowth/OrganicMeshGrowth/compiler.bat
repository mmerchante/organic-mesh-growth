cd shaders

%VK_SDK_PATH%\Bin\glslangValidator.exe -V graphics.vert
move vert.spv graphics.vert.spv
%VK_SDK_PATH%\Bin\glslangValidator.exe -V graphics.frag
move frag.spv graphics.frag.spv

%VK_SDK_PATH%\Bin\glslangValidator.exe -V compute.comp
move comp.spv compute.comp.spv

