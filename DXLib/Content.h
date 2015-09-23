// Base class for any content that needs to be loaded from storage during runtime

#ifndef Content_h__
#define Content_h__

#include <string>

#include "contentParameters.h"

#include <d3d11.h>

class ContentManager;

class Content
{
	friend class ContentManager; //For loading and unloading
public:
	explicit Content();
	//DO NOT USE FOR DEALLOCATION/REMOVAL! Use Unload() instead!
	virtual ~Content();

	virtual bool IsLoaded() const { return path.size() > 0; }

private:
	std::string path; //TODO: Use something else for faster hashing/access?
	int refCount; //If Unload is called and refCount == 0 it's safe to fully unload this content

	//************************************
	// Method:		Load
	// FullName:	Content::Load
	// Access:		virtual public 
	// Returns:		bool - whether or not the content was loaded successfully
	// Qualifier:	
	// Argument:	const std::string& path - path with extension e.g. "Path/To/Asset.filetype"
	// Argument:	ID3D11Device* device - if needed, otherwise pass nullptr
	// Argument:	ContentManager* contentManager - if needed, otherwise pass nullptr
	// Argument:	ContentParameters* contentParameters - optional argument containing a subclass of contentParameters
	// Description:	Loads this content into memory
	//************************************
    virtual bool Load(const std::string& path, ID3D11Device* device, ContentManager* contentManager = nullptr, ContentParameters* contentParameters = nullptr) = 0;
	//************************************
	// Method:		Unload
	// FullName:	Content::Unload
	// Access:		virtual public 
	// Returns:		void
	// Qualifier:	
	// Argument:	ContentManager* contentManager - current contentManager. Used so any "accessory content" can be unloaded
	// Description:	Unallocate/delete memory here. Called from ContentManager::Unload
	//************************************
    virtual void Unload(ContentManager* contentManager = nullptr) = 0;
};

#endif // Content_h__
