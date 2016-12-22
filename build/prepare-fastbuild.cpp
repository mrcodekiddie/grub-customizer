/*
 * Copyright (C) 2010-2011 Daniel Richter <danielrichter2007@web.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "common-build.hpp"
#include <memory>
#include <string>
#include <iostream>
#include <list>
#include <map>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

namespace GcBuild
{
	std::string makeReadable(std::string str)
	{
		str = replaceStringContents("\n", "⏷", str);
		str = replaceStringContents("\t", "⏵", str);
		return str;
	}

	std::string renderString(std::string const& str)
	{
		std::string rendered;
		if (str.size() < 20) {
			rendered = makeReadable(str);
		} else {
			rendered = makeReadable(str.substr(0, 8)) + " ... " + makeReadable(str.substr(str.size() - 7));
		}
		return rendered;
	}

	class AbstractContent
	{
		public: enum class RenderDest
		{
			HEADER,
			SOURCE
		};

		public: std::list<std::shared_ptr<AbstractContent>> children;

		public: virtual ~AbstractContent(){}

		public: void dumpTree(unsigned int indent = 0)
		{
			unsigned int pos = 0;
			for (auto& child : this->children) {
				std::cout << GcBuild::stringRepeat("  ", indent) << "(" << ++pos << ") " << child->describe() << std::endl;
				child->dumpTree(indent + 1);
			}
		}

		public: virtual std::string describe() = 0;

		public: virtual bool isContainer()
		{
			return false;
		}

		public: virtual bool isEndOfContainer()
		{
			return false;
		}

		public: virtual void optimize()
		{
			for (auto& child : this->children) {
				child->optimize();
			}
		}

		public: virtual std::string render(std::list<std::string> path)
		{
			std::string result;
			for (auto& child : this->children) {
				result += child->render(path);
			}
			return result;
		}

		public: virtual std::string getPathPart()
		{
			return "";
		}

		public: virtual bool isFunction()
		{
			return false;
		}

		public: std::map<std::shared_ptr<AbstractContent>, std::list<std::string>> findFunctions(
			std::list<std::string> currentPath = {}
		) {
			if (this->getPathPart() != "") {
				currentPath.push_back(this->getPathPart());
			}
			std::map<std::shared_ptr<AbstractContent>, std::list<std::string>> result;
			for (auto& child : this->children) {
				if (child->isFunction()) {
					result[child] = currentPath;
				}
				auto childItems = child->findFunctions(currentPath);
				result.insert(childItems.begin(), childItems.end());
			}
			return result;
		}

		public: virtual void groupChars()
		{
			std::list<std::shared_ptr<AbstractContent>> newList;
			for (auto& child : this->children) {
				child->groupChars(); // recursion

				if (newList.size()) {
					try {
						newList.back()->mergeIfType("char", child);
					} catch (std::runtime_error const& e) {
						newList.push_back(child);
					}
				} else {
					newList.push_back(child);
				}
			}
			this->children = newList;
		}

		public: virtual void mergeIfType(std::string const& type, std::shared_ptr<AbstractContent> other)
		{
			throw std::runtime_error("merge not supported");
		}
	};

	class Namespace : public AbstractContent
	{
		public: std::string name;
		public: virtual ~Namespace(){}
		public: std::string describe()
		{
			return "namespace " + this->name;
		}
		public: virtual bool isContainer()
		{
			return true;
		}

		public: virtual std::string render(std::list<std::string> path) override
		{
			path.push_back(this->getPathPart());
			return "namespace " + this->name + " {" + AbstractContent::render(path) + "}";
		}

		public: virtual std::string getPathPart() override
		{
			return this->name;
		}
	};

	class Property : public AbstractContent
	{
		public: virtual ~Property(){}
		public: std::string describe()
		{
			return "property";
		}
		public: virtual bool isContainer()
		{
			return true;
		}
	};

	/**
	 * generic container with brackets
	 */
	class Bracket : public AbstractContent
	{
		public: std::string openingBracket;
		public: std::string closingBracket;
		public: virtual ~Bracket(){}
		public: Bracket(std::string const& bracket) : openingBracket(bracket) {
			if (bracket.size() == 0) {
				throw std::runtime_error("got zero sized bracket");
			}
			switch (bracket[0]) {
				case '{': this->closingBracket = '}'; break;
				case '(': this->closingBracket = ')'; break;
				case '[': this->closingBracket = ']'; break;
			}
		}
		public: std::string describe()
		{
			return "bracket: " + this->openingBracket + " ... " + this->closingBracket;
		}
		public: virtual bool isContainer()
		{
			return true;
		}

		public: virtual std::string render(std::list<std::string> path) override
		{
			return this->openingBracket + AbstractContent::render(path) + this->closingBracket;
		}
	};

	/**
	 * dummy object to notify about the end of a bracket
	 */
	class ClosingBracket : public AbstractContent
	{
		public: std::string bracket;
		public: virtual ~ClosingBracket(){}
		public: ClosingBracket(std::string const& bracket) : bracket(bracket) {}
		public: std::string describe()
		{
			return "Closing bracket: " + this->bracket;
		}
		public: virtual bool isEndOfContainer()
		{
			return true;
		}
	};

	class GenericCode : public AbstractContent
	{
		public: std::string data;
		public: std::string type;

		public: GenericCode(std::string const& type, std::string const& data)
			: type(type), data(data)
		{}

		public: virtual ~GenericCode(){}

		public: virtual std::string describe()
		{
			std::string desc = this->type + " [" + GcBuild::intToString(this->data.size()) + "]";

			desc += " : " + renderString(this->data);

			return desc;
		}

		public: virtual std::string render(std::list<std::string> path) override
		{
			return this->data;
		}

		public: virtual void mergeIfType(std::string const& type, std::shared_ptr<AbstractContent> other)
		{
			if (this->type != type) {
				throw std::runtime_error("wrong type");
			}
			auto otherAsGenericCode = std::dynamic_pointer_cast<GenericCode>(other);
			if (otherAsGenericCode == nullptr) {
				throw std::runtime_error("other object is not generic code");
			}
			if (otherAsGenericCode->type != type) {
				throw std::runtime_error("other object is wrong type");
			}
			this->data += otherAsGenericCode->data;
		}
	};

	class Function : public AbstractContent
	{
		public: std::list<std::shared_ptr<AbstractContent>> beforeAccess;
		public: std::shared_ptr<GenericCode> access;
		public: std::list<std::shared_ptr<AbstractContent>> returnValue;
		public: std::string name;
		public: std::list<std::shared_ptr<AbstractContent>> intermediateStuff; // between ) and {
		public: std::shared_ptr<Bracket> parameterList;
		public: virtual ~Function(){}
		public: std::string describe()
		{
			return "function " + this->name;
		}
		public: virtual bool isContainer()
		{
			return true;
		}

		public: virtual std::string render(RenderDest dest, std::list<std::string> path)
		{
			std::string content;
			if (dest == AbstractContent::RenderDest::HEADER) {
				for (auto& before : this->beforeAccess) {
					content += before->render(path);
				}
			}

			if (this->access && dest == AbstractContent::RenderDest::HEADER) {
				content += this->access->render(path);
			}
			for (auto& retValPart : this->returnValue) {
				content += retValPart->render(path);
			}
			if (dest == AbstractContent::RenderDest::SOURCE) {
				content += join(path, "::") + "::";
			}
			content += this->name;
			content += this->parameterList->render(path);
			if (this->name != path.back() || dest == RenderDest::SOURCE) {
				for (auto& interPart : this->intermediateStuff) {
					// source must not have the keyword "override"
					if (dest == RenderDest::SOURCE && this->isWord(interPart, "override")) {
						continue;
					}
					content += interPart->render(path);
				}
			}
			if (dest == AbstractContent::RenderDest::SOURCE) {
				content += "{" + AbstractContent::render(path) + "}";
			} else {
				content += ";";
			}
			return content;
		}

		private: bool isWord(std::shared_ptr<AbstractContent> obj, std::string word)
		{
			auto objectAsGenericCode = std::dynamic_pointer_cast<GenericCode>(obj);
			if (objectAsGenericCode == nullptr) {
				return false;
			}
			if (objectAsGenericCode->type != "word") {
				return false;
			}
			if (objectAsGenericCode->data != word) {
				return false;
			}
			return true;
		}

		public: virtual std::string render(std::list<std::string> path) override
		{
			return this->render(AbstractContent::RenderDest::HEADER, path);
		}

		public: virtual bool isFunction()
		{
			return true;
		}
	};

	class Class : public AbstractContent
	{
		public: std::string name;
		public: std::string properties;
		public: virtual ~Class(){}
		public: std::string describe()
		{
			std::string desc = "class " + this->name;
			if (this->properties.size()) {
				desc += " | Properties: " + renderString(this->properties);
			}
			return desc;
		}
		public: virtual bool isContainer()
		{
			return true;
		}

		public: virtual void optimize()
		{
			auto splittedChildren = this->splitChildren();
			this->children = {};
			for (auto& childList : splittedChildren) {
				this->children.push_back(this->convert(childList));
			}

			AbstractContent::optimize();
		}

		/**
		 * split by entity ends like ; and {}
		 */
		private: std::list<std::list<std::shared_ptr<AbstractContent>>> splitChildren()
		{
			std::list<std::list<std::shared_ptr<AbstractContent>>> result;

			result.push_back({});

			for (auto& child : this->children) {
				result.back().push_back(child);

				auto childAsBracket = dynamic_cast<Bracket*>(child.get());
				if (childAsBracket != nullptr && childAsBracket->openingBracket == "{") {
					result.push_back({});
				}

				auto childAsGeneric = dynamic_cast<GenericCode*>(child.get());
				if (childAsGeneric != nullptr && childAsGeneric->type == "commandSeparator") {
					result.push_back({});
				}
			}

			return result;
		}

		private: std::shared_ptr<AbstractContent> convert(std::list<std::shared_ptr<AbstractContent>> list)
		{
			bool isFunction = false;
			if (std::dynamic_pointer_cast<Bracket>(list.back()) && std::dynamic_pointer_cast<Bracket>(list.back())->openingBracket == "{") {
				for (auto& child : list) {
					auto childAsBracket = std::dynamic_pointer_cast<Bracket>(child);
					if (childAsBracket && childAsBracket->openingBracket == "(") {
						isFunction = true;
					}
				}
			}

			if (isFunction) {
				// use first round brackets as parameter list
				std::shared_ptr<Bracket> parameterList = nullptr;
				for (auto& child : list) {
					if (std::dynamic_pointer_cast<Bracket>(child) && std::dynamic_pointer_cast<Bracket>(child)->openingBracket == "(") {
						parameterList = std::dynamic_pointer_cast<Bracket>(child);
						break;
					}
				}

				auto result = std::make_shared<Function>();
				result->children = list.back()->children; // use contents of closing bracket
				list.pop_back();
				// read until parameter list
				while (list.size() > 0 && list.back() != parameterList) {
					result->intermediateStuff.push_front(list.back());
					list.pop_back();
				}
				if (list.size() == 0) {
					throw std::runtime_error("function read failed - parameter list not found");
				}
				result->parameterList = std::dynamic_pointer_cast<Bracket>(list.back());
				list.pop_back();

				// read until first word (function name)
				while (list.size() > 0 && (!std::dynamic_pointer_cast<GenericCode>(list.back()) || std::dynamic_pointer_cast<GenericCode>(list.back())->type != "word")) {
					list.pop_back();
				}
				if (list.size() == 0) {
					throw std::runtime_error("function read failed - name not found");
				}
				result->name = dynamic_cast<GenericCode*>(list.back().get())->data;
				list.pop_back();

				// read return value
				while (list.size() > 0 && (!std::dynamic_pointer_cast<GenericCode>(list.back()) || std::dynamic_pointer_cast<GenericCode>(list.back())->type != "accessControl")) {
					result->returnValue.push_front(list.back());
					list.pop_back();
				}
				if (list.size() != 0) {
					result->access = std::dynamic_pointer_cast<GenericCode>(list.back());
					list.pop_back();
				}

				while (list.size() != 0) {
					result->beforeAccess.push_front(list.back());
					list.pop_back();
				}

				return result;
			} else {
				auto result = std::make_shared<Property>();
				result->children = list;
				return result;
			}
		}

		public: virtual std::string render(std::list<std::string> path) override
		{
			path.push_back(this->getPathPart());
			return "class " + this->name + this->properties + "\n{ " + AbstractContent::render(path) + "}";
		}

		public: virtual std::string getPathPart() override
		{
			return this->name;
		}

	};

	class File : public AbstractContent
	{
		public: virtual ~File(){}
		public: virtual std::string describe()
		{
			return "file";
		}

		public: std::string renderSource()
		{
			std::string result;

			for (auto& functionAndPath : this->findFunctions()) {
				result += std::dynamic_pointer_cast<Function>(functionAndPath.first)->render(RenderDest::SOURCE, functionAndPath.second);
			}

			return result;
		}
	};
	
	class Parser
	{
		public: std::string content;
		private: size_t pos = 0;
		private: std::string wordChars;

		public: Parser(std::string const& content = "")
			: content(content)
		{
			this->wordChars = buildCharString('a', 'z') + buildCharString('A', 'Z') + buildCharString('0', '9') + "_";
		}

		public: std::shared_ptr<File> parse()
		{
			this->pos = 0;

			std::shared_ptr<File> result = std::make_shared<File>();

			std::list<std::shared_ptr<AbstractContent>> containerPath;
			containerPath.push_back(result);

			std::shared_ptr<AbstractContent> nextPart = nullptr;
			while ((nextPart = this->readNextPart())) {
				if (!nextPart->isEndOfContainer()) {
					containerPath.back()->children.push_back(nextPart);
				}
				if (nextPart->isContainer()) {
					containerPath.push_back(nextPart);
				}
				if (nextPart->isEndOfContainer()) {
					containerPath.pop_back();
				}
			}

			return result;
		}

		private: std::shared_ptr<AbstractContent> readNextPart()
		{
			std::shared_ptr<AbstractContent> result = nullptr;
			(result = this->readComment()) ||
			(result = this->readString()) ||
			(result = this->readOpeningBracket()) ||
			(result = this->readClosingBracket()) ||
			(result = this->readPreprocessor()) ||
			(result = this->readNamespace()) ||
			(result = this->readClass()) ||
			(result = this->readAccessControl()) ||
			(result = this->readWord()) ||
			(result = this->readCommandSeparator()) ||
			(result = this->readChar());

			return result;
		}

		private: std::shared_ptr<GenericCode> readComment()
		{
			std::shared_ptr<GenericCode> result = nullptr;

			if (this->content.substr(this->pos, 2) == "/*") {
				size_t end = this->content.find("*/", this->pos + 2) + 2;
				result = std::make_shared<GenericCode>("multiline-comment", this->content.substr(this->pos, end - this->pos));
				this->pos = end;
			} else if (this->content.substr(this->pos, 2) == "//") {
				size_t end = this->content.find_first_of('\n', this->pos + 2) + 1;
				result = std::make_shared<GenericCode>("singleline-comment", this->content.substr(this->pos, end - this->pos));
				this->pos = end;
			}

			return result;
		}

		private: std::shared_ptr<GenericCode> readString()
		{
			std::shared_ptr<GenericCode> result = nullptr;

			std::string nextChar = this->content.substr(this->pos, 1);
			if (nextChar == "'" || nextChar == "\"") {
				result = std::make_shared<GenericCode>("string", nextChar);
				this->pos++;
				bool goOn = false;
				do {
					goOn = false;
					size_t end = this->content.find_first_of(nextChar + "\\", this->pos);
					if (end == -1) {
						throw std::runtime_error("found unterminated string");
					}
					if (this->content[end] == '\\') {
						// if we found an escape char, read it and the following char and coninue this loop...
						result->data += this->content.substr(this->pos, end - this->pos + 2);
						this->pos = end + 2;
						goOn = true;
					} else {
						//... otherwise end here
						result->data += this->content.substr(this->pos, end - this->pos + 1);
						this->pos = end + 1;
					}
				} while (goOn);
			}

			return result;
		}

		private: std::shared_ptr<GenericCode> readPreprocessor()
		{
			std::shared_ptr<GenericCode> result = nullptr;

			if (this->content.substr(this->pos, 1) == "#") {
				size_t end = this->content.find_first_of('\n', this->pos + 2) + 1;
				result = std::make_shared<GenericCode>("preprocessor", this->content.substr(this->pos, end - this->pos));
				this->pos = end;
			}

			return result;
		}

		private: std::shared_ptr<Namespace> readNamespace()
		{
			std::shared_ptr<Namespace> result = nullptr;

			if (this->getNextWord() == "namespace") {
				size_t end = this->content.find_first_of(";{", this->pos + std::string("namespace").size());
				if (this->content.substr(end, 1) == ";") {
					// end if there follows ";" instead of "{"
					return nullptr;
				}
				size_t contentBracketPos = end;
				pos += std::string("namespace").size();
				result = std::make_shared<Namespace>();
				result->name = GcBuild::trim(this->content.substr(this->pos, contentBracketPos - this->pos - 1));
				this->pos = contentBracketPos + 1;
			}

			return result;
		}

		private: std::shared_ptr<Class> readClass()
		{
			std::shared_ptr<Class> result = nullptr;

			if (this->getNextWord() == "class") {
				pos += std::string("class").size();
				result = std::make_shared<Class>();
				size_t contentBracketPos = this->content.find_first_of('{', this->pos);
				std::string nameAndProperties = this->content.substr(this->pos, contentBracketPos - this->pos - 1);
				size_t colonPos = nameAndProperties.find_first_of(':');
				if (colonPos == -1) { // no properties set
					result->name = nameAndProperties;
				} else {
					result->name = GcBuild::trim(nameAndProperties.substr(0, colonPos));
					result->properties = GcBuild::trim(nameAndProperties.substr(colonPos));
				}
				this->pos = contentBracketPos + 1;
			}

			return result;
		}

		private: std::shared_ptr<Bracket> readOpeningBracket()
		{
			std::shared_ptr<Bracket> result = nullptr;

			std::string bracket = this->content.substr(this->pos, 1);
			if (bracket == "{" || bracket == "(" || bracket == "[") {
				result = std::make_shared<Bracket>(bracket);
				pos++;
			}

			return result;
		}

		private: std::shared_ptr<ClosingBracket> readClosingBracket()
		{
			std::shared_ptr<ClosingBracket> result = nullptr;

			std::string bracket = this->content.substr(this->pos, 1);
			if (bracket == "}" || bracket == ")" || bracket == "]") {
				result = std::make_shared<ClosingBracket>(bracket);
				pos++;
			}

			return result;
		}

		private: std::shared_ptr<GenericCode> readAccessControl()
		{
			std::shared_ptr<GenericCode> accessControl = nullptr;

			std::string wordStr = this->getNextWord();
			if (wordStr == "private" || wordStr == "protected" || wordStr == "public") {
				this->pos += wordStr.size();
				size_t end = this->content.find_first_of(':', this->pos);
				accessControl = std::make_shared<GenericCode>("accessControl", wordStr + this->content.substr(this->pos, end - this->pos + 1));
				this->pos = end  + 1;
			}

			return accessControl;
		}

		private: std::shared_ptr<GenericCode> readWord()
		{
			std::shared_ptr<GenericCode> word = nullptr;

			std::string wordStr = this->getNextWord();
			if (wordStr != "") {
				word = std::make_shared<GenericCode>("word", wordStr);
				this->pos += wordStr.size();
			}

			return word;
		}

		/**
		 * just read a char - should be used as fallback
		 */
		private: std::shared_ptr<GenericCode> readCommandSeparator()
		{
			std::string data = this->content.substr(this->pos, 1);
			if (data != ";") {
				return nullptr;
			}
			this->pos++;
			return std::make_shared<GenericCode>("commandSeparator", data);
		}

		/**
		 * just read a char - should be used as fallback
		 */
		private: std::shared_ptr<GenericCode> readChar()
		{
			std::string data = this->content.substr(this->pos, 1);
			if (data == "") {
				return nullptr;
			}
			this->pos++;
			return std::make_shared<GenericCode>("char", data);
		}

		private: std::string getNextWord()
		{
			std::string result;

			std::string firstChar = this->content.substr(this->pos, 1);

			if (firstChar.find_first_of(this->wordChars) != -1) {
				size_t end = this->content.find_first_not_of(this->wordChars, this->pos);
				result = this->content.substr(this->pos, end - this->pos);
			}

			return result;
		}
	};

	class Optimizer
	{
		private: std::string root;
		public: Optimizer(std::string const& root)
			: root(root)
		{}

		public: void optimize(
			std::string const& inputFile,
			std::string const& headerDest,
			std::string const& implDest
		) {
			auto content = GcBuild::getFileContents(this->root + "/" + inputFile);

			auto file = std::make_shared<Parser>(content)->parse();
			file->groupChars();
			file->dumpTree();
//			file->optimize();

//			putFileContents(this->root + "/" + headerDest, file->render({}));
//			putFileContents(this->root + "/" + implDest, file->renderSource());
		}
	};

	void prepareDirectory(std::string const& basePath, std::string const& file)
	{
		auto parts = split(file, "/");
		parts.pop_back();
		std::string path = "";
		for (auto& part : parts) {
			path += part + "/";
			mkdir((basePath + "/" + path).c_str(), 0777);
		}
	}
}

int main(int argc, char** argv)
{
	if (argc < 2) {
		std::cerr << "I need the root path of the project as argument #1" << std::endl;
		return 1;
	}

	auto optimizer = std::make_shared<GcBuild::Optimizer>(argv[1]);

	auto destPath = std::string(argv[1]) + "/build/fastbuild";

	for (int i = 2; i < argc; i++) {
		std::cout << "preparing " << argv[i] << std::endl;

		mkdir(destPath.c_str(), 0777);
		GcBuild::prepareDirectory(destPath, argv[i]);
		optimizer->optimize(
			argv[i],
			destPath + "/" + argv[i],
			GcBuild::substituteSuffix(destPath + "/" + argv[i], "cpp")
		);
	}
}
