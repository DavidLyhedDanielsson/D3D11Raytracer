#ifndef ContentLoader_h__
#define ContentLoader_h__

#include <d3d11.h>

#include "content.h"
#include "contentParameters.h"
#include "contentCreationParameters.h"

#include <unordered_map>

#include "logger.h"

class ContentManager
{
public:
	ContentManager();
	//Don't allow copying since it would require shared_ptr and whatnot (for the destructor)
	ContentManager(const ContentManager&&) = delete; //C++11! Wooo!
	ContentManager& operator=(const ContentManager&) = delete;
	virtual ~ContentManager();

	void Init(ID3D11Device* device)
	{
		this->device = device;
	}

	//************************************
	// Method:		Load
	// FullName:	ContentManager::Load
	// Access:		public 
	// Returns:		T*
	// Qualifier:	
	// Argument:	const std::string& path
	// Description:	Loads content. If the content at the given path has been loaded before it will return a pointer to that instance.
	// Use *Load<type>(path) to create a copy
	//************************************
	template<typename T>
	T* Load(const std::string& path, ContentParameters* contentParameters = nullptr)
	{
		//If path is empty a new object should be created from the content parameters, thus there's no need to look in the map
		if(path != "")
		{
			auto iter = contentMap.find(path);

			if(iter != contentMap.end())
			{
				iter->second->refCount++;
				return static_cast<T*>(iter->second);
			}
		}

		Content* newContent = static_cast<Content*>(new T);

		if(newContent->Load(path, device, this, contentParameters))
		{
			newContent->refCount = 1;
			if(path != "")
			{
				newContent->path = path;
				contentMap.insert(std::pair<std::string, Content*>(path, newContent));
			}
			else
			{
				ContentCreationParameters* creationParameters = dynamic_cast<ContentCreationParameters*>(contentParameters);

				if(creationParameters == nullptr)
				{
					Logger::LogLine(LOG_TYPE::FATAL, "Path is \"\" but couldn't cast contentParameters to ContentCreationParameters!");
					delete newContent;
					return nullptr;
				}

				if(creationParameters->uniqueID == "")
				{
					Logger::LogLine(LOG_TYPE::FATAL, "No uniqueID set for content without path!");
					delete newContent;
					return nullptr;
				}

				contentMap.insert(std::pair<std::string, Content*>(creationParameters->uniqueID, newContent));
			}

			return static_cast<T*>(newContent);
		}

		delete newContent;
		return nullptr;
	}

	//************************************
	// Method:		Unload
	// FullName:	ContentManager::Unload
	// Access:		public 
	// Returns:		void
	// Qualifier:	
	// Description:	Unloads all content loaded by this ContentManager
	//************************************
	void Unload();
	//************************************
	// Method:		Unload
	// FullName:	ContentManager::Unload
	// Access:		public 
	// Returns:		void
	// Qualifier:	
	// Argument:	const std::string& path
	// Description:	Unloads the given content
	//************************************
	void Unload(const std::string& path);
	//************************************
	// Method:		Unload
	// FullName:	ContentManager::Unload
	// Access:		public 
	// Returns:		void
	// Qualifier:	
	// Argument:	Content* content
	// Description:	Unloads the given content
	//************************************
	void Unload(Content* content);
private:
	std::unordered_map<std::string, Content*> contentMap; //Use map if there are memory issues

	ID3D11Device* device;
};

#endif // ContentLoader_h__
