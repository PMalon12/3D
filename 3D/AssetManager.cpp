#include "AssetManager.hpp"

AssetManager::AssetManager()
{
}

AssetManager::~AssetManager()
{
}

Asset* AssetManager::prepareAsset(Asset::Type pType, String<128>& pPath, String<32>& pName)
{
	switch (pType)
	{
	case Asset::Font: {
		auto ins = fontList.insert(std::make_pair(pName, Font(pPath, pName)));
		return &ins.first->second; }
	case Asset::Mesh: {
		auto ins2 = modelList.try_emplace(pName, pPath, pName);
		return &ins2.first->second; }
	case Asset::Model: {
		auto ins3 = modelList.try_emplace(pName, pPath, pName);
		return &ins3.first->second; }
	case Asset::Texture2D: {
		auto ins4 = texture2DList.insert(std::make_pair(pName, Texture2D(pPath, pName)));
		return &ins4.first->second; }
	}
}

void AssetManager::loadAssets(String128 & assetListFilePath)
{
	std::ifstream file;
	file.open(assetListFilePath.getString());

	std::string section;

	while (!file.eof())
	{
		std::string line;
		while (std::getline(file, line))
		{
			if (line.length() < 3)
				continue;

			std::string key;
			std::string value;

			auto getUntil = [](std::string& src, char delim)-> std::string {
				auto ret = src.substr(0, src.find(delim) == -1 ? 0 : src.find(delim));
				src.erase(0, src.find(delim)+1);
				return ret;
			};


			if (line == "[Texture]")
			{
				std::string name, path, mip;
				std::getline(file, key, '=');
				std::getline(file, value);
				path = value;
				std::getline(file, key, '=');
				std::getline(file, value);
				name = value;
				std::getline(file, key, '=');
				std::getline(file, value);
				mip = value;

				auto find = Engine::assets.texture2DList.find(String32(name.c_str()));
				if (find != Engine::assets.texture2DList.end())
				{
					std::cout << "Texture \"" << name << "\" already loaded" << std::endl;
					continue;
				}

				if (mip == "true")
				{
					auto tex = Engine::assets.prepareTexture(String128(path.c_str()), String128(name.c_str()));
					if (tex->doesExist())
					{
						tex->load();
					}
					else
					{
						std::cout << "Texture at " << path << " does not exist" << std::endl;
					}
					tex->makeGLAsset();
					tex->glData->loadToGPU();
				}
				else
				{
					auto tex = Engine::assets.prepareTexture(String128(path.c_str()), String128(name.c_str()));
					if (tex->doesExist())
					{
						tex->load();
					}
					else
					{
						std::cout << "Texture at \"" << path << "\" does not exist" << std::endl;
					}
					tex->makeGLAsset();
					tex->glData->loadToGPU();
				}
				
			}
			else if (line == "[Model]")
			{
				std::string line;
				std::getline(file, line);
				std::string name, path, ext, limit, albedo, normal, specular, metallic, roughness;
				std::vector<std::string> paths;
				std::vector<u32> limits;

				auto place = [&](std::string key, std::string value) -> bool {
					if (key == "path")
					{
						path = value;
						return true;
					}
					else if (key == "name")
					{
						name = value;
						return true;
					}
					else if (key == "ext")
					{
						ext = value;
						return true;
					}
					else if (key == "limit")
					{
						limit = value;
						return true;
					}
					else if (key == "albedo")
					{
						albedo = value;
						return true;
					}
					else if (key == "normal")
					{
						normal = value;
						return true;
					}
					else if (key == "specular")
					{
						specular = value;
						return true;
					}
					else if (key == "metallic")
					{
						metallic = value;
						return true;
					}
					else if (key == "roughness")
					{
						roughness = value;
						return true;
					}
					return false;
				};

				key = getUntil(line, '=');

				int lod = 0;

				while(key != "")
				{
					value = line;
					place(key, value);
					std::getline(file, line);

					if (key == "path")
						paths.push_back(path);
					else if (key == "limit")
						limits.push_back(std::stoi(limit));

					key = getUntil(line, '=');
				}

				auto find = Engine::assets.modelList.find(String32(name.c_str()));
				if (find != Engine::assets.modelList.end())
				{
					std::cout << "Mesh \"" << name << "\" already loaded" << std::endl;
					continue;
				}

				auto model = Engine::assets.prepareModel(String128(path.c_str()), String32(name.c_str()));
				if (model->doesExist())
				{
					model->lodPaths = paths;
					model->lodLimits = limits;
					model->load();
					for (auto itr = model->triLists.begin(); itr != model->triLists.end(); ++itr)
					{
						for (auto it = itr->begin(); it != itr->end(); ++it)
						{
							it->matMeta.albedo.name.setToChars(albedo.c_str());
							it->matMeta.normal.name.setToChars(normal.c_str());
							if (specular.length() != 0)
								it->matMeta.specularMetallic.name.setToChars(specular.c_str());
							else
								it->matMeta.specularMetallic.name.setToChars(metallic.c_str());

							it->matMeta.roughness.name.setToChars(roughness.c_str());

							it->matMeta.albedo.glTex = Engine::assets.get2DTexGL(it->matMeta.albedo.name);
							it->matMeta.normal.glTex = Engine::assets.get2DTexGL(it->matMeta.normal.name);
							it->matMeta.specularMetallic.glTex = Engine::assets.get2DTexGL(it->matMeta.specularMetallic.name);

							if (roughness.length() != 0)
							{
								it->matMeta.roughness.glTex = Engine::assets.get2DTexGL(it->matMeta.roughness.name);
							}

							/// TODO: AO texture
						}
					}
					Engine::assets.modelManager.pushModelToBatch(*model);
				}
				else
				{
					std::cout << "Mesh at \"" << path << "\" does not exist" << std::endl;
				}
			}
			else if (line == "[Font]")
			{
				std::string name, path, ext;
				std::getline(file, key, '=');
				std::getline(file, value);
				path = value;
				std::getline(file, key, '=');
				std::getline(file, value);
				name = value;

				auto find = Engine::assets.fontList.find(String32(name.c_str()));
				if (find != Engine::assets.fontList.end())
				{
					std::cout << "Font \"" << name << "\" already loaded" << std::endl;
					continue;
				}

				auto font = Engine::assets.prepareFont(String128(path.c_str()), String128(name.c_str()));
				if (font->doesExist())
				{
					font->load();
				}
				else
				{
					std::cout << "Font at \"" << path << "\" does not exist" << std::endl;
				}
			}
		}
	}
	file.close();
}