#include "common.h"
#include "GL32Renderer.h"
#include "Integrator.h"
#include "PositionComponent.h"
#include "OrientationComponent.h"
#include "ModelComponent.h"
#include "PlayerCameraComponent.h"

class GL32ModelProperty : public Property {
public:
	const GLuint vao;
	const std::vector<GLuint> vbos;
	GL32ModelProperty(const GLuint vao, const std::vector<GLuint> &vbos) : vao(vao), vbos(vbos) {}
	GL32ModelProperty(const GLuint vao, std::vector<GLuint> &&vbos) : vao(vao), vbos(vbos) {}
};

bool GL32Renderer::IsSupported() {
	GL32Renderer renderer(nullptr);
	try {
		renderer.InitGL();
		GLint major, minor;
		if(!GL(glGetIntegerv(GL_MAJOR_VERSION, &major))) { ENGINE_THROW("failed to get OpenGL major version"); }
		if(!GL(glGetIntegerv(GL_MINOR_VERSION, &minor))) { ENGINE_THROW("failed to get OpenGL minor version"); }
		/* if OpenGL version is 3.2 or greater */
		if(!(major > 3 || (major == 3 && minor >= 2))) {
			std::stringstream msg;
			msg << "actual OpenGL version is " << major << '.' << minor;
			ENGINE_THROW(msg.str());
		}
		renderer.Destroy();
	} catch(std::exception &) {
		renderer.Destroy();
		return false;
	}
	return true;
}

void GL32Renderer::InitGL() {
	if(!SDL(SDL_Init(SDL_INIT_EVERYTHING))) { ENGINE_THROW("failed to init SDL"); }
	if(!SDL(SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3))) { ENGINE_THROW("failed to set major version attribute"); }
	if(!SDL(SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2))) { ENGINE_THROW("failed to set minor version attribute"); }
	if(!SDL(SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY))) { ENGINE_THROW("failed to set profile mask attribute"); }
	if(!SDL(SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1))) { ENGINE_THROW("failed to set double buffer attribute"); }
	//if(!SDL(SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1))) { ENGINE_THROW("failed to set multisample buffers attribute"); }
	//if(!SDL(SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 8))) { ENGINE_THROW("failed to set multisample samples attribute"); }
	if(!SDL(window = SDL_CreateWindow("Polar Engine", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height,
		SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE))) {
		ENGINE_THROW("failed to create window");
	}
	if(!SDL(context = SDL_GL_CreateContext(window))) { ENGINE_THROW("failed to create OpenGL context"); }
	if(!SDL(SDL_GL_SetSwapInterval(1))) { ENGINE_THROW("failed to set swap interval"); }
	if(!SDL(SDL_SetRelativeMouseMode(SDL_TRUE))) { ENGINE_THROW("failed to set relative mouse mode"); }

	glewExperimental = GL_TRUE;
	GLenum err = glewInit();

	if(err != GLEW_OK) { ENGINE_THROW("GLEW: glewInit failed"); }

	/* GLEW cals glGetString(EXTENSIONS) which
	 * causes GL_INVALID_ENUM on GL 3.2+ core contexts
	 */
	glGetError();

	GL(glEnable(GL_DEPTH_TEST));
	GL(glEnable(GL_BLEND));
	GL(glEnable(GL_CULL_FACE));
	//GL(glEnable(GL_MULTISAMPLE));
	GL(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
	GL(glCullFace(GL_BACK));
}

void GL32Renderer::Init() {
	width = 1280;
	height = 720;
	fovy = 70;
	zNear = 0.05f;
	InitGL();
	SetClearColor(Point4(0.02f, 0.05f, 0.1f, 1));
	//ENGINE_OUTPUT(engine->systems.Get<AssetManager>()->Get<TextAsset>("hello").text << '\n');
}

void GL32Renderer::Update(DeltaTicks &dt, std::vector<Object *> &objects) {
	SDL_Event event;
	while(SDL_PollEvent(&event)) {
		HandleSDL(event);
	}
	SDL_ClearError();

	auto integrator = engine->systems.Get<Integrator>();
	float alpha = integrator->Accumulator().Seconds();

	glm::mat4 cameraView;
	for(auto object : objects) {
		auto camera = object->Get<PlayerCameraComponent>();
		if(camera != nullptr) {
			cameraView = glm::translate(cameraView, -camera->distance.Temporal(alpha).To<glm::vec3>());
			cameraView *= glm::toMat4(camera->orientation);

			auto orient = object->Get<OrientationComponent>();
			if(orient != nullptr) {
				cameraView *= glm::toMat4(orient->orientation);
			}

			cameraView = glm::translate(cameraView, -camera->position.Temporal(alpha).To<glm::vec3>());

			auto pos = object->Get<PositionComponent>();
			if(pos != nullptr) {
				cameraView = glm::translate(cameraView, -pos->position.Temporal(alpha).To<glm::vec3>());
			}
		}
	}

	for(unsigned int i = 0; i < nodes.size(); ++i) {
		auto &node = nodes[i];
		GL(glBindFramebuffer(GL_FRAMEBUFFER, node.fbo));
		GL(glUseProgram(node.program));
		GL(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

		for(unsigned int iBuffer = 0; iBuffer < node.buffers.size(); ++iBuffer) {
			auto buffer = node.buffers[iBuffer];
			GL(glActiveTexture(GL_TEXTURE0 + iBuffer));
			GL(glBindTexture(GL_TEXTURE_2D, buffer));
			GLint locBuffer;
			GL(locBuffer = glGetUniformLocation(node.program, node.bufferNames[iBuffer].c_str()));
			GL(glUniform1i(locBuffer, iBuffer));
		}

		switch(i) {
		case 0: {
			GLint locModelView;
			GL(locModelView = glGetUniformLocation(node.program, "u_modelView"));

			for(auto object : objects) {
				auto model = object->Get<ModelComponent>();
				if(model != nullptr) {
					auto property = model->Get<GL32ModelProperty>();
					if(property != nullptr) {
						glm::mat4 modelView = cameraView;

						auto pos = object->Get<PositionComponent>();
						if(pos != nullptr) { modelView = glm::translate(modelView, pos->position.Temporal(alpha).To<glm::vec3>()); }

						auto orient = object->Get<OrientationComponent>();
						if(orient != nullptr) { modelView *= glm::toMat4(glm::inverse(orient->orientation)); }

						GLenum drawMode = GL_TRIANGLES;
						switch(model->type) {
						case GeometryType::Triangles:
							drawMode = GL_TRIANGLES;
							break;
						case GeometryType::TriangleStrip:
							drawMode = GL_TRIANGLE_STRIP;
							break;
						}

						GL(glUniformMatrix4fv(locModelView, 1, GL_FALSE, glm::value_ptr(modelView)));
						GL(glBindVertexArray(property->vao));
						GL(glDrawArrays(drawMode, 0, static_cast<GLsizei>(model->points.size())));
					}
				}
			}
			break;
		}
		default:
			/*GLuint vbo;
			GL(glGenBuffers(2, &vbo));

			std::vector<Point> points = {
				Point(-1, -1, 0, 1),
				Point(1, -1, 0, 1)
			};
			GL(glBindBuffer(GL_ARRAY_BUFFER, vbo));
			GL(glBufferData(GL_ARRAY_BUFFER, sizeof(Point) * points.size(), points.data(), GL_STATIC_DRAW));
			GL(glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, NULL));*/

			GL(glEnableVertexAttribArray(0));
			glBegin(GL_QUADS);
			glVertex2f(-1, -1);
			glVertex2f( 1, -1);
			glVertex2f( 1,  1);
			glVertex2f(-1,  1);
			glEnd();
			break;
		}
	}

	SDL(SDL_GL_SwapWindow(window));
}

void GL32Renderer::Destroy() {
	SDL(SDL_GL_DeleteContext(context));
	SDL(SDL_DestroyWindow(window));
	SDL(SDL_GL_ResetAttributes());
	SDL(SDL_Quit());
}

void GL32Renderer::ObjectAdded(Object *object) {
	auto model = object->Get<ModelComponent>();
	if(model != nullptr) {
		GLuint vao;
		GL(glGenVertexArrays(1, &vao));
		GL(glBindVertexArray(vao));

		/* location   attribute
		*
		*        0   vertex
		*        1   normal
		*/

		GLuint vbos[2];
		GL(glGenBuffers(2, vbos));

		GL(glBindBuffer(GL_ARRAY_BUFFER, vbos[0]));
		GL(glBufferData(GL_ARRAY_BUFFER, sizeof(Point3) * model->points.size(), model->points.data(), GL_STATIC_DRAW));
		GL(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL));

		GL(glBindBuffer(GL_ARRAY_BUFFER, vbos[1]));
		GL(glBufferData(GL_ARRAY_BUFFER, sizeof(Point3) * model->normals.size(), model->normals.data(), GL_STATIC_DRAW));
		GL(glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL));

		GL(glEnableVertexAttribArray(0));
		GL(glEnableVertexAttribArray(1));

		std::vector<GLuint> vbosVector;
		for(unsigned int i = 0; i < sizeof(vbos) / sizeof(*vbos); ++i) {
			vbosVector.emplace_back(vbos[i]);
		}

		model->Add<GL32ModelProperty>(vao, vbosVector);
	}
}

void GL32Renderer::ObjectRemoved(Object *object) {
	auto model = object->Get<ModelComponent>();
	if(model != nullptr) {
		auto property = model->Get<GL32ModelProperty>();
		if(property != nullptr) {
			for(auto vbo : property->vbos) {
				GL(glDeleteBuffers(1, &vbo));
			}
			GL(glDeleteVertexArrays(1, &property->vao));
			model->Remove<GL32ModelProperty>();
		}
	}
}

void GL32Renderer::HandleSDL(SDL_Event &event) {
	Key key;
	switch(event.type) {
	case SDL_QUIT:
		engine->Quit();
		break;
	case SDL_WINDOWEVENT:
		switch(event.window.event) {
		case SDL_WINDOWEVENT_RESIZED:
			width = event.window.data1;
			height = event.window.data2;
			GL(glViewport(0, 0, width, height));
			Project(nodes.front().program);
			break;
		}
		break;
	case SDL_KEYDOWN:
		if(event.key.repeat == 0) {
			key = mkKeyFromSDL(event.key.keysym.sym);
			engine->systems.Get<EventManager>()->Fire("keydown", &key);
		}
		break;
	case SDL_KEYUP:
		key = mkKeyFromSDL(event.key.keysym.sym);
		engine->systems.Get<EventManager>()->Fire("keyup", &key);
		break;
	case SDL_MOUSEMOTION:
		Point2 delta(event.motion.xrel, event.motion.yrel);
		engine->systems.Get<EventManager>()->Fire("mousemove", &delta);
		break;
	}
}

void GL32Renderer::Project(GLuint programID) {
	GLint locProjection;
	GL(locProjection = glGetUniformLocation(programID, "u_projection"));
	//glm::mat4 projection = glm::infinitePerspective(fovy, static_cast<float>(width) / static_cast<float>(height), zNear);
	glm::mat4 projection = glm::perspective(fovy, static_cast<float>(width) / static_cast<float>(height), zNear, 80.0f);
	GL(glUniformMatrix4fv(locProjection, 1, GL_FALSE, glm::value_ptr(projection)));
}

void GL32Renderer::SetClearColor(const Point4 &color) {
	GL(glClearColor(color.x, color.y, color.z, color.w));
}

void GL32Renderer::MakePipeline(const std::vector<std::string> &names) {
	std::vector<ShaderAsset> assets;
	nodes.clear();

	for(auto &name : names) {
		INFOS("loading shader asset `" << name << '`');
		auto asset = engine->systems.Get<AssetManager>()->Get<ShaderAsset>(name);
		assets.emplace_back(asset);
		nodes.emplace_back(MakeProgram(asset));
	}

	for(unsigned int i = 0; i < nodes.size() - 1; ++i) {
		auto &asset = assets[i], &nextAsset = assets[i + 1];
		auto &node = nodes[i], &nextNode = nodes[i + 1];

		GL(glGenFramebuffers(1, &node.fbo));
		GL(glBindFramebuffer(GL_FRAMEBUFFER, node.fbo));

		std::vector<GLenum> drawBuffers;
		int colorAttachment = 0;

		for(auto &in : nextAsset.ins) {
			GLuint buffer;
			GL(glGenTextures(1, &buffer));
			GL(glBindTexture(GL_TEXTURE_2D, buffer));
			switch(in.type) {
			case ProgramInOutType::Color:
				GL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL));
				GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
				GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
				GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
				GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
				GL(glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + colorAttachment, buffer, 0));
				drawBuffers.emplace_back(GL_COLOR_ATTACHMENT0 + colorAttachment);
				break;
			case ProgramInOutType::Depth:
				GL(glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL));
				GL(glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, buffer, 0));
				break;
			default:
				ENGINE_THROW("invalid program output type");
				break;
			}
			nextNode.buffers.emplace_back(buffer);
			nextNode.bufferNames.emplace_back(in.name);
		}

		GL(glDrawBuffers(drawBuffers.size(), drawBuffers.data()));

		GLenum status;
		GL(status = glCheckFramebufferStatus(GL_FRAMEBUFFER));

		if(status != GL_FRAMEBUFFER_COMPLETE) {
			std::stringstream msg;
			msg << "framebuffer status incomplete (0x" << std::hex << status << ')';
			ENGINE_THROW(msg.str());
		}
	}

	/* upload projection matrix to first stage of pipeline */
	GL(glUseProgram(nodes.front().program));
	Project(nodes.front().program);
}

void GL32Renderer::Use(const std::string &name) {
	INFOS("loading shader asset `" << name << '`');
	auto asset = engine->systems.Get<AssetManager>()->Get<ShaderAsset>(name);
	auto programID = MakeProgram(asset);

	if(!GL(glUseProgram(programID))) { ENGINE_THROW("failed to use program"); }
	//activeProgram = programID;

	/* set projection matrix in new program */
	Project(programID);
}

GLuint GL32Renderer::MakeProgram(ShaderAsset &asset) {
	std::vector<GLuint> ids;
	for(auto &shader : asset.shaders) {
		GLenum type = 0;
		switch(shader.type) {
		case ShaderType::Vertex:
			type = GL_VERTEX_SHADER;
			break;
		case ShaderType::Fragment:
			type = GL_FRAGMENT_SHADER;
			break;
		default:
			ENGINE_THROW("invalid shader type");
		}

		GLuint id;
		if(!GL(id = glCreateShader(type))) { ENGINE_THROW("failed to create shader"); }

		const GLchar *src = shader.source.c_str();
		const GLint len = static_cast<GLint>(shader.source.length());
		if(!GL(glShaderSource(id, 1, &src, &len))) { ENGINE_THROW("failed to upload shader source"); }
		if(!GL(glCompileShader(id))) { ENGINE_THROW("shader compilation is unsupported on this platform"); }

		GLint status;
		if(!GL(glGetShaderiv(id, GL_COMPILE_STATUS, &status))) { ENGINE_THROW("failed to get shader compilation status"); }

		if(status == GL_FALSE) {
			GLint infoLen;
			if(!GL(glGetShaderiv(id, GL_INFO_LOG_LENGTH, &infoLen))) { ENGINE_THROW("failed to get shader info log length"); }

			if(infoLen > 0) {
				char *infoLog = new char[infoLen];
				if(!GL(glGetShaderInfoLog(id, infoLen, NULL, infoLog))) { ENGINE_THROW("failed to get shader info log"); }
				ENGINE_ERROR(infoLog);
				delete[] infoLog;
			}
			ENGINE_THROW("failed to compile shader");
		}

		ids.push_back(id);
	}

	GLuint programID;
	if(!GL(programID = glCreateProgram())) { ENGINE_THROW("failed to create program"); }

	for(auto id : ids) {
		if(!GL(glAttachShader(programID, id))) { ENGINE_THROW("failed to attach shader to program"); }

		/* flag shader for deletion */
		if(!GL(glDeleteShader(id))) { ENGINE_THROW("failed to flag shader for deletion"); }
	}

	if(!GL(glLinkProgram(programID))) { ENGINE_THROW("program linking is unsupported on this platform"); }

	GLint status;
	if(!GL(glGetProgramiv(programID, GL_LINK_STATUS, &status))) { ENGINE_THROW("failed to get program linking status"); }

	if(status == GL_FALSE) {
		GLint infoLen;
		if(!GL(glGetProgramiv(programID, GL_INFO_LOG_LENGTH, &infoLen))) { ENGINE_THROW("failed to get program info log length"); }

		if(infoLen > 0) {
			char *infoLog = new char[infoLen];
			if(!GL(glGetProgramInfoLog(programID, infoLen, NULL, infoLog))) { ENGINE_THROW("failed to get program info log"); }
			ENGINE_ERROR(infoLog);
			delete[] infoLog;
		}
		ENGINE_THROW("failed to link program");
	}
	return programID;
}
