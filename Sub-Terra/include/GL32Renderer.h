#pragma once

#include <functional>
#include <boost/array.hpp>
#include <boost/container/vector.hpp>
#include <boost/container/flat_set.hpp>
#include <boost/unordered_set.hpp>
#include <boost/unordered_map.hpp>
#include "sdl.h"
#include "gl.h"
#include "Renderer.h"
#include "ShaderProgramAsset.h"
#include "ModelComponent.h"
#include "Text.h"

struct PipelineNode {
	GLuint program;
	GLuint fbo = 0;
	boost::unordered_set<std::string> uniforms;
	boost::unordered_map<std::string, GLuint> outs;
	boost::unordered_map<std::string, std::string> ins;
	boost::unordered_map<std::string, GLuint> globalOuts;
	boost::unordered_map<std::string, std::string> globalIns;
	PipelineNode(GLuint program) : program(program) {}
};

struct GL32ModelProperty : public Property {
	GLuint vao;
	std::vector<GLuint> vbos;
	GLsizei numVertices = 0;
	GLsizei capacity = 0;

	inline friend bool operator<(const GL32ModelProperty &left, const GL32ModelProperty &right) {
		return left.capacity < right.capacity;
	}
};

struct GL32TextProperty : public Property {
	GLuint texture;
};

template<typename T> struct SharedPtrLess : public std::binary_function<boost::shared_ptr<T>, boost::shared_ptr<T>, bool> {
	inline bool operator()(const boost::shared_ptr<T> &left, const boost::shared_ptr<T> &right) const {
		return *left < *right;
	}
};

class GL32Renderer : public Renderer {
private:
	SDL_Window *window;
	SDL_GLContext context;
	boost::container::vector<std::string> pipelineNames;
	boost::container::vector<PipelineNode> nodes;
	boost::container::flat_multiset<boost::shared_ptr<GL32ModelProperty>, SharedPtrLess<GL32ModelProperty>> modelPropertyPool;

	GLuint viewportVAO;
	GLuint textProgram;

	uint32_t time = 0;

	boost::shared_ptr<Destructor> fpsDtor;
	IDType fpsID = 0;

	void Init() override final;
	void Update(DeltaTicks &) override final;

	inline boost::shared_ptr<GL32ModelProperty> GetPooledModelProperty(const GLsizei required) {
		if(modelPropertyPool.empty()) {
			GL32ModelProperty prop;

			GL(glGenVertexArrays(1, &prop.vao));
			GL(glBindVertexArray(prop.vao));

			/* location   attribute
			 *
			 *        0   vertex
			 *        1   normal
			 */

			prop.vbos.resize(2);
			GL(glGenBuffers(2, &prop.vbos[0]));

			GL(glBindBuffer(GL_ARRAY_BUFFER, prop.vbos[0]));
			GL(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL));

			GL(glBindBuffer(GL_ARRAY_BUFFER, prop.vbos[1]));
			GL(glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL));

			GL(glEnableVertexAttribArray(0));
			GL(glEnableVertexAttribArray(1));

			return boost::make_shared<GL32ModelProperty>(prop);
		} else {
			auto dummy = boost::make_shared<GL32ModelProperty>();
			dummy->capacity = required;

			auto it = modelPropertyPool.lower_bound(dummy);
			if(it == modelPropertyPool.cend()) { it = modelPropertyPool.begin(); }

			auto prop = *it;
			modelPropertyPool.erase(it);
			return prop;
		}
	}

	inline void UploadModel(boost::shared_ptr<ModelComponent> model) {
		auto normals = model->CalculateNormals();
		auto numVertices = normals.size();
		auto dataSize = sizeof(Point3) * numVertices;
		auto prop = GetPooledModelProperty(numVertices);

		if(numVertices > 0) {
			if(GLsizei(numVertices) > prop->capacity) {
				GL(glBindBuffer(GL_ARRAY_BUFFER, prop->vbos[0]));
				GL(glBufferData(GL_ARRAY_BUFFER, dataSize, model->points.data(), GL_DYNAMIC_DRAW));

				GL(glBindBuffer(GL_ARRAY_BUFFER, prop->vbos[1]));
				GL(glBufferData(GL_ARRAY_BUFFER, dataSize, normals.data(), GL_DYNAMIC_DRAW));

				prop->capacity = numVertices;
			} else {
				GL(glBindBuffer(GL_ARRAY_BUFFER, prop->vbos[0]));
				GL(glBufferSubData(GL_ARRAY_BUFFER, 0, dataSize, model->points.data()));

				GL(glBindBuffer(GL_ARRAY_BUFFER, prop->vbos[1]));
				GL(glBufferSubData(GL_ARRAY_BUFFER, 0, dataSize, normals.data()));
			}
		}

		prop->numVertices = numVertices;

		model->Add<GL32ModelProperty>(prop);
		//model->points.clear();
		//model->points.shrink_to_fit();
	}

	inline void ComponentAdded(IDType id, const std::type_info *ti, boost::weak_ptr<Component> ptr) override final {
		if(ti == &typeid(ModelComponent)) {
			auto model = boost::static_pointer_cast<ModelComponent>(ptr.lock());
			UploadModel(model);
		} else if(ti == &typeid(Text)) {
			auto text = boost::static_pointer_cast<Text>(ptr.lock());
			GL32TextProperty prop;

			GL(glGenTextures(1, &prop.texture));
			GL(glBindTexture(GL_TEXTURE_2D, prop.texture));

			GLint format = GL_RGBA;
			GL(glTexImage2D(GL_TEXTURE_2D, 0, format, text->surface->w, text->surface->h, 0, format, GL_UNSIGNED_BYTE, text->surface->pixels));
			GL(glGenerateMipmap(GL_TEXTURE_2D));
			GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
			GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
			GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
			GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST));

			text->Add<GL32TextProperty>(prop);
		}
	}

	inline void ComponentRemoved(IDType id, const std::type_info *ti) override final {
		if(ti == &typeid(ModelComponent)) {
			auto model = engine->GetComponent<ModelComponent>(id);
			if(model != nullptr) {
				auto prop = model->Get<GL32ModelProperty>().lock();
				if(prop) {
					modelPropertyPool.emplace(prop);
				}
			}
		} else if(ti == &typeid(Text)) {
			auto text = engine->GetComponent<Text>(id);
			if(text != nullptr) {
				auto prop = text->Get<GL32TextProperty>().lock();
				if(prop) {
					GL(glDeleteTextures(1, &prop->texture));
				}
			}
		}
	}

	inline glm::mat4 CalculateProjection() {
		auto heightF = static_cast<float>(height);
		auto fovy = 2.0f * glm::atan(heightF, 2.0f * pixelDistanceFromScreen) + fovPlus;
		auto projection = glm::perspective(fovy, static_cast<float>(width) / heightF, zNear, zFar);
		//auto projection = glm::infinitePerspective(fovy, static_cast<float>(width) / h, zNear);
		return projection;
	}

	inline void Project(GLuint programID) {
		GL(glUseProgram(programID));

		GLint locProjection;
		GL(locProjection = glGetUniformLocation(programID, "u_projection"));
		if(locProjection == -1) { return; } /* -1 if uniform does not exist in program */

		auto projection = CalculateProjection();
		GL(glUniformMatrix4fv(locProjection, 1, GL_FALSE, glm::value_ptr(projection)));
	}

	void InitGL();
	void HandleSDL(SDL_Event &);
	GLuint MakeProgram(std::shared_ptr<ShaderProgramAsset>);
public:
	boost::unordered_map<std::string, float> uniformsFloat;
	boost::unordered_map<std::string, Point3> uniformsPoint3;
	float fps = 60.0f;

	static bool IsSupported();
	GL32Renderer(Polar *engine, const boost::container::vector<std::string> &names) : Renderer(engine), pipelineNames(names) {}
	~GL32Renderer();
	void MakePipeline(const boost::container::vector<std::string> &) override final;

	inline void SetClearColor(const Point4 &color) override final {
		GL(glClearColor(color.x, color.y, color.z, color.w));
	}

	void SetUniform(const std::string &name, float x) {
		uniformsFloat[name] = x;
		for(auto &node : nodes) {
			if(node.uniforms.find(name) != node.uniforms.end()) {
				GL(glUseProgram(node.program));
				GLint loc;
				GL(loc = glGetUniformLocation(node.program, name.c_str()));
				GL(glUniform1f(loc, x));
			}
		}
	}

	void SetUniform(const std::string &name, Point3 p) {
		uniformsPoint3[name] = p;
		for(auto &node : nodes) {
			if(node.uniforms.find(name) != node.uniforms.end()) {
				GL(glUseProgram(node.program));
				GLint loc;
				GL(loc = glGetUniformLocation(node.program, name.c_str()));
				GL(glUniform3f(loc, p.x, p.y, p.z));
			}
		}
	}
};
