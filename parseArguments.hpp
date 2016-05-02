/*
Copyright 2016 Petr Kubica

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http ://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifndef _PARSE_ARGUMENTS
#define _PARSE_ARGUMENTS

#include <map>
#define LINE_LENGTH 80

using namespace std;

class ParseArguments
{
	class Param
	{
		string toUpper(string str);
		string m_sLong_variant_argument;
		string m_sShort_variant_argument;
		string m_sHelp_description;
		bool m_bHas_value;
		bool m_bRequired;
		bool m_bUse_equation_operator;

	public:
		Param(string argument, string simple, string help, bool param, bool required, bool equ): m_sLong_variant_argument(argument), m_sShort_variant_argument(simple), m_sHelp_description(help), m_bHas_value(param), m_bRequired(required), m_bUse_equation_operator(equ) {}
		string composeForPrinting();
		string composeForUsagePrinting();
		string getHelpDescription();
		bool isRequired();
		bool hasValue();
	};


public:
	ParseArguments(bool eq) : m_bEquation_format(eq)
	{
		m_iIndentation = 2;
		m_sDescription = "";
		m_sProgram_name = "";
		m_bExists_param_required = false;
		m_iMax_count_char_arg = 0;
		m_iSize_of_line_help_description = 0;
		addArgument("help", "h", "show this help message and exit", false, false);
	}

	void inline writeSpaces(int count);
	void inline writeDescription(string description, int size, int indent);
	void help();
	void calculateColumn(int length);
	void addArgument(string argument, string simple, string help, bool param, bool required);
	void parseValueFromParam(char *param, char **value);
	void parseArgs(int argc, char *argv []);
	void setProgramName(string program_name);
	void setDescription(string description);
	void checkRequiredParams();
	bool checkIfSet(string param);
	string getParamValue(string param);

private:
	int m_iIndentation;
	string m_sDescription;
	string m_sProgram_name;

	bool m_bEquation_format;
	bool m_bExists_param_required;
	int m_iMax_count_char_arg;
	int m_iSize_of_line_help_description;
	map<string, Param> m_mapsCparamParam_info;
	map<string, string> m_mapssShort_to_long_params;
	map<string, string> m_map_ssEntered_param_value;
};



#endif // !_PARSE_ARGUMENTS
