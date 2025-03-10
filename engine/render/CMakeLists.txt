#--------------------------------------------------------------------------
# render
#--------------------------------------------------------------------------

SET(files_render_display
	window.h
	window.cc
	)
SOURCE_GROUP("display" FILES ${files_render_display})

SET(files_render_render
	model.h
	model.cc
	renderdevice.h
	renderdevice.cc
	shaderresource.h
	shaderresource.cc
	textureresource.h
	textureresource.cc
	lightserver.h
	lightserver.cc
	cameramanager.h
	cameramanager.cc
	debugrender.h
	debugrender.cc
	grid.h
	grid.cc
	lightsources.h
	physics.h
	physics.cc
	resourceid.h
	particlesystem.cc
	particlesystem.h
	
	# external single header libs
	stb_image.h
	stb_image_write.h
	json.hpp
	gltf.h
	)
SOURCE_GROUP("render" FILES ${files_render_render})

SET(files_render_input
	input/gamepad.h
	input/gamepad.cc
	input/inputserver.h
	input/inputserver.cc
	input/key.h
	input/key.cc
	input/keyboard.h
	input/mouse.h
	)
SOURCE_GROUP("input" FILES ${files_render_input})

SET(files_render_entityManagement
	entityManagement/entityid.h
	entityManagement/entity.h
	entityManagement/physicsid.h
	entityManagement/chunkAllocator.h
	entityManagement/componentBase.h
	entityManagement/components.h
	entityManagement/world.h
	entityManagement/componentType.h
	entityManagement/entityType.h
	)
SOURCE_GROUP("entityManagement" FILES ${files_render_entityManagement})



SET(files_render
	${files_render_display}
	${files_render_render}
	${files_render_pch}
	${files_render_input}
	${files_render_entityManagement})

SET(files_pch ../config.h ../config.cc)
SOURCE_GROUP("pch" FILES ${files_pch})
ADD_LIBRARY(render STATIC ${files_render} ${files_pch})
TARGET_PCH(render ../)
ADD_DEPENDENCIES(render exts imgui glew glfw glm_static)
TARGET_LINK_LIBRARIES(render PUBLIC engine exts glew glfw imgui ${OPENGL_LIBS} glm_static)