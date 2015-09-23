#include "dictionary.h"

Dictionary::Dictionary()
{

}

void Dictionary::AddEntry(const std::string& entry)
{
	DictionaryChapter& chapter = chapters[tolower(entry[0])];

	chapter.AddEntry(entry);
}

const DictionaryEntry* Dictionary::Find(const std::string text) const
{
	std::map<char, DictionaryChapter>::const_iterator chapterIter = chapters.find(static_cast<char>(tolower(text[0])));
	if(chapterIter != chapters.end())
		return chapterIter->second.Find(text);

	return nullptr;
}

std::vector<const DictionaryEntry*> Dictionary::Match(const std::string text) const
{
	std::vector<const DictionaryEntry*> matches;

	std::map<char, DictionaryChapter>::const_iterator chapterIter = chapters.find(static_cast<char>(tolower(text[0])));
	while(chapterIter != chapters.end())
	{
		auto newMatches = chapterIter->second.Match(text);

		matches.reserve(matches.size() + newMatches.size());
		matches.insert(matches.end(), std::make_move_iterator(newMatches.begin()), std::make_move_iterator(newMatches.end()));
		++chapterIter;
	}

	return matches;
}
