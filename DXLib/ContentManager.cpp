#include "contentManager.h"

#include <string>

#include "logger.h"

ContentManager::ContentManager()
{

}

ContentManager::~ContentManager()
{
	Unload();
}

void ContentManager::Unload()
{
	for(auto iter : contentMap)
	{
		iter.second->Unload(this);
	}

	for(auto iter : contentMap)
	{
		delete iter.second;
		iter.second = nullptr;
	}

	contentMap.clear();
}

void ContentManager::Unload(const std::string& path)
{
	auto iter = contentMap.find(path);
	if(iter->second == nullptr)
		return; //Already unloaded

	if(!iter->second->IsLoaded())
		return;

	if(iter == contentMap.end())
	{
		Logger::LogLine(LOG_TYPE::WARNING, "Trying to unload conten that has already been unloaded or hasn't been loaded at all");
		return;
	}

	iter->second->refCount--;
	if(iter->second->refCount > 0)
		return;

	iter->second->Unload(this);
	iter->second = nullptr;
	delete iter->second;

	contentMap.erase(iter->first);
}

void ContentManager::Unload(Content* content)
{
	std::string path = content->path;

	if(path == "") //"Local" content, it's not in contentMap
	{
		Logger::LogLine(LOG_TYPE::WARNING, "Trying to unload content where path = \"\". Make sure to only load content through ContentManager");
		return;
	}

	auto iter = contentMap.find(path);

	if(iter->second == nullptr)
		return; //Already unloaded

	iter->second->refCount--;
	if(iter->second->refCount > 0)
		return; //This content is used somewhere else

	iter->second->Unload(this);
	delete contentMap[iter->first];

	contentMap.erase(path);
}