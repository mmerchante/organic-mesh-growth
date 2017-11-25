cd shaders

%VK_SDK_PATH%\Bin\glslangValidator.exe -V graphics.vert
move vert.spv graphics.vert.spv

%VK_SDK_PATH%\Bin\glslangValidator.exe -V graphics.frag
move frag.spv graphics.frag.spv

%VK_SDK_PATH%\Bin\glslangValidator.exe -V kernel.comp
move comp.spv kernel.comp.spv

%VK_SDK_PATH%\Bin\glslangValidator.exe -V generator.comp
move comp.spv generator.comp.spv