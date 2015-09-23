#ifndef OPENGLWINDOW_DICTIONARY_H
#define OPENGLWINDOW_DICTIONARY_H

#include "dictionaryChapter.h"

#include <string>
#include <map>

class Dictionary
{
public:
	Dictionary();
	~Dictionary() = default;

	void AddEntry(const std::string& entry);

	const DictionaryEntry* Find(const std::string text) const;
	std::vector<const DictionaryEntry*> Match(const std::string text) const;
private:
	std::map<char, DictionaryChapter> chapters;
};

#endif //OPENGLWINDOW_DICTIONARY_H
