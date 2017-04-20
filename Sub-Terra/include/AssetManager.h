#pragma once

#include <boost/unordered_map.hpp>
#include "Asset.h"
#include "FileSystem.h"
#include "Serializer.h"

class AssetManager : public System {
private:
	boost::unordered_map<std::string, std::shared_ptr<Asset>> cache;
public:
	static std::string GetAssetsDir() {
#ifdef _WIN32
		return FileSystem::GetAppDir() + "/assets";
#endif
#ifdef __APPLE__
		return FileSystem::GetAppDir() + "/Contents/Resources";
#endif
	}

	template<typename T> static std::string GetDir() {
		static_assert(std::is_base_of<Asset, T>::value, "AssetManager::GetDir requires typename of type Asset");
		return GetAssetsDir() + "/" + AssetName<T>();
	}

	template<typename T> static std::string GetPath(const std::string &name) {
		static_assert(std::is_base_of<Asset, T>::value, "AssetManager::GetPath requires typename of type Asset");
		return GetDir<T>() + "/" + name + ".asset";
	}

	template<typename T> std::vector<std::string> List() {
		static_assert(std::is_base_of<Asset, T>::value, "AssetManager::List requires typename of type Asset");
		auto ls = FileSystem::ListDir(GetDir<T>());
		std::transform(ls.begin(), ls.end(), ls.begin(), [] (std::string s) {
			return s.substr(0, s.size() - 6);
		});
		return ls;
	}

	static bool IsSupported() { return true; }
	AssetManager(Polar *engine) : System(engine) {}

	template<typename T, typename ...Ts> std::shared_ptr<T> Get(const std::string name, Ts && ...args) {
		static_assert(std::is_base_of<Asset, T>::value, "AssetManager::Get requires typename of type Asset");
		auto path = GetPath<T>(name);
		if(cache.find(path) == cache.end()) {
			auto asset = std::make_shared<T>(std::forward<Ts>(args)...);
			std::istringstream ss(FileSystem::ReadFile(path));
			Deserializer deserializer(ss);
			deserializer >> *asset;
			cache.emplace(path, std::static_pointer_cast<Asset>(asset));
		}
		return std::static_pointer_cast<T>(cache.at(path));
	}
};
