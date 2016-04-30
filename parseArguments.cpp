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

#include <iostream>
#include <deque>
#include <vector>
#include <map>
#include <string>
#include <locale>
#include <algorithm>
#include <iterator>
#include <sstream>
#include <cstring>

#include "parseArguments.hpp"

using namespace std;

/// <summary>
/// Transform text from lower case to upper case.
/// </summary>
/// <param name="str">Text in lower form.</param>
/// <returns>Text in upper form.</returns>
string ParseArguments::Param::toUpper(string str)
{
	locale loc;
	for (string::size_type i = 0; i < str.length(); ++i)
		str[i] = toupper(str[i], loc);
	return str;
}

/// <summary>
/// Composing argument for printing for example: -h,--help or --source=SOURCE etc.
/// </summary>
/// <returns>Print form of parameter for help</returns>
string ParseArguments::Param::composeForPrinting()
{
	string l_sComposed_argument = "";
	if (m_sShort_variant_argument != "")
	{
		l_sComposed_argument += string("-") + m_sShort_variant_argument;
		if (m_bHas_value) l_sComposed_argument += string(" ") + toUpper(m_sLong_variant_argument);
		l_sComposed_argument += string(", ");
	}
	l_sComposed_argument += string("--") + m_sLong_variant_argument;
	if (m_bHas_value && m_bUse_equation_operator)	l_sComposed_argument += string("=") + toUpper(m_sLong_variant_argument);
	if (m_bHas_value && !m_bUse_equation_operator)	l_sComposed_argument += string(" ") + toUpper(m_sLong_variant_argument);
	return l_sComposed_argument;
}

/// <summary>
/// Composing argument for printing usage - available parameters [-h,--help] [--optional=value] --required=value
/// </summary>
/// <returns>Print form of parameter for usage</returns>
string ParseArguments::Param::composeForUsagePrinting()
{
	string help = "";
	if (!m_bRequired) help += string("[");
	help += composeForPrinting();
	if (!m_bRequired)	help += string("]");
	return help;
}

/// <summary>
/// Gets the help description of argument.
/// </summary>
/// <returns>Get description of parameter for help</returns>
string ParseArguments::Param::getHelpDescription()
{
	return m_sHelp_description;
}

/// <summary>
/// Check if parameter is reguired.
/// </summary>
/// <returns></returns>
bool ParseArguments::Param::isRequired()
{ 
	return m_bRequired; 
}

/// <summary>
/// Check if parameter has value. For example --parameter=VALUE
/// </summary>
/// <returns></returns>
bool ParseArguments::Param::hasValue()
{ 
	return m_bHas_value; 
}

/// <summary>
/// Writes the spaces on cout - for indenting text.
/// </summary>
/// <param name="count">Count of spaces</param>
void inline ParseArguments::writeSpaces(int count)
{ 
	for (int i = 0; i < count; ++i) cout << " "; 
}

/// <summary>
/// Writes the description of parameter and indent from left to column.
/// </summary>
/// <param name="description">The description.</param>
/// <param name="size">Size of column.</param>
/// <param name="indent">Indent from left.</param>
void inline ParseArguments::writeDescription(string description, int size, int indent)
{
	vector<string> l_vecsDescription_words;

	istringstream iss(description);
	copy(istream_iterator<string>(iss),
		istream_iterator<string>(),
		back_inserter(l_vecsDescription_words));

	int free_space = size;
	for (vector<string>::iterator it = l_vecsDescription_words.begin(); it != l_vecsDescription_words.end(); ++it)
	{
		size_t length = it->length();
		if ((free_space - length) > 0)
		{
			cout << *it << " ";
			free_space -= length + 1;
		}
		else
		{
			//TODO: missing check: word in description is longer than space
			cout << endl;
			writeSpaces(indent);
			cout << *it << " ";
			free_space = size - (length + 1);
		}

	}
	cout << endl;
}

/// <summary>
/// Print help of program to stdout.
/// </summary>
void ParseArguments::help()
{
	string usage = "";
	if (m_sProgram_name != "")
		usage += "usage: " + m_sProgram_name;
	else
		usage += "usage:";

	size_t usage_len = usage.length();
	cout << usage;
	string line = "";
	int line_free = LINE_LENGTH - usage_len;
	
	///help first !!
	string help_arg = " " + m_mapsCparamParam_info.at("help").composeForUsagePrinting();
	size_t help_arg_length = help_arg.length();
	cout << help_arg;
	line_free -= help_arg_length;

	for (map<string, Param>::iterator it = m_mapsCparamParam_info.begin(); it != m_mapsCparamParam_info.end(); ++it)
	{
		if (it->first == "help") continue;
		string help_arg = " " + it->second.composeForUsagePrinting();
		int help_arg_length = help_arg.length();
		if ((line_free - help_arg_length) < 0)
		{
			cout << endl;
			writeSpaces(usage_len);
			cout << help_arg;
			line_free = LINE_LENGTH - usage_len - help_arg_length;
		}
		else
		{
			cout << help_arg;
			line_free -= help_arg_length;
		}
	}
	
	if (m_sDescription == "")
	{
		cout << endl << endl;
	}
	else
	{
		cout << endl << endl << m_sDescription << endl;
	}
	
	if (m_bExists_param_required)
	{
		cout << "Required arguments:" << endl;
		for (map<string, Param>::iterator it = m_mapsCparamParam_info.begin(); it != m_mapsCparamParam_info.end(); ++it)
		{
			if (!it->second.isRequired()) continue;


			string arg = it->second.composeForPrinting();
			writeSpaces(m_iIndentation);
			cout << arg;
			writeSpaces(m_iIndentation + (m_iMax_count_char_arg - arg.length()));
			writeDescription(it->second.getHelpDescription(), m_iSize_of_line_help_description, 2 * m_iIndentation + m_iMax_count_char_arg);
		}
		cout << endl;
	}

	cout << "Optional arguments:" << endl;
	///help first !!
	string arg = m_mapsCparamParam_info.at("help").composeForPrinting();
	writeSpaces(m_iIndentation);
	cout << arg;
	writeSpaces(m_iIndentation + (m_iMax_count_char_arg - arg.length()));
	writeDescription(m_mapsCparamParam_info.at("help").getHelpDescription(), m_iSize_of_line_help_description, 2 * m_iIndentation + m_iMax_count_char_arg);

	for (map<string, Param>::iterator it = m_mapsCparamParam_info.begin(); it != m_mapsCparamParam_info.end(); ++it)
	{
		if (it->second.isRequired()) continue;
		if (it->first == "help") continue;

		string arg = it->second.composeForPrinting();
		writeSpaces(m_iIndentation);
		cout << arg;
		writeSpaces(m_iIndentation + (m_iMax_count_char_arg - arg.length()));
		writeDescription(it->second.getHelpDescription(), m_iSize_of_line_help_description, 2 * m_iIndentation + m_iMax_count_char_arg);
	}
}

/// <summary>
/// Calculate size of right column for parameters description - check maximum size of left column based on parameters length.
/// </summary>
/// <param name="length">The length of one parameter.</param>
void ParseArguments::calculateColumn(int length)
{
	if ((m_iIndentation + length) > m_iMax_count_char_arg)
	{
		m_iMax_count_char_arg = length;
		m_iSize_of_line_help_description = LINE_LENGTH - 2 * m_iIndentation - m_iMax_count_char_arg;
	}
}

/// <summary>
/// Adds the parameter for program. Add full length argument, optional short form, help description and if is required.
/// </summary>
/// <param name="full_form">Full form of parameter like 'parameter_name' (showed as --parameter_name).</param>
/// <param name="short_form">Short form of parameter like 'h' (showed as -h).</param>
/// <param name="help_description">Help description.</param>
/// <param name="has_a_value">if set to <c>true</c> parser will expect a value after parameter, for example: --param=value or --param value.</param>
/// <param name="is_required">if set to <c>true</c> paramameter will be expected when launching -> if not present then exception.</param>
void ParseArguments::addArgument(string full_form, string short_form, string help_description, bool has_a_value, bool is_required)
{
	pair<map<string, Param>::iterator, bool> ret;
	ret = m_mapsCparamParam_info.insert(pair<string, Param>(full_form, Param(full_form, short_form, help_description, has_a_value, is_required, m_bEquation_format)));
	if (!ret.second)
	{
		throw OptionsException("Parameter --" + full_form + " already exist!");
	}

	if (short_form != "")
	{
		pair<map<string, string>::iterator, bool> ret2;
		ret2 = m_mapssShort_to_long_params.insert(pair<string, string>(short_form, full_form));
		if (!ret2.second)
		{
			throw OptionsException("Short parameter -" + short_form + string(" already exist!"));
		}
	}

	calculateColumn(m_mapsCparamParam_info.at(full_form).composeForPrinting().length());
	if (is_required) m_bExists_param_required = true;
}

/// <summary>
/// Function for split 'param=value' to param, value.
/// </summary>
/// <param name="param">Parsing 'param=value' and store there only 'param'</param>
/// <param name="value">For storing value.</param>
void ParseArguments::parseValueFromParam(char *param, char **value)
{
	int i = 0;
	while (param[i] != '=')
	{
		if (param[i] == '\0') break;
		++i;
	}
	if (param[i] == '=')
	{
		param[i] = '\0';
		if (param[i + 1] == '\0')
		{
			*value = NULL;
		}
		else
		{
			*value = &(param[i + 1]);
		}
	}
	else
	{
		*value = NULL;
	}
}

/// <summary>
/// Parsing input arguments: check form, parsing values, save values and present of parameters, check required parameter on present.
/// </summary>
/// <param name="argc">The argc of program.</param>
/// <param name="argv">The argv of program.</param>
void ParseArguments::parseArgs(int argc, char *argv [])
{
	for (int i = 1; i < argc; ++i)
	{
		if (strlen(argv[i]) >= 2)
		{
			if (argv[i][0] == '-')
			{
				pair<map<string, string>::iterator, bool> ret;
				if (argv[i][1] == '-' && strlen(argv[i]) > 2)
				{
					char *param = &(argv[i][2]);
					char *value;
					if (m_bEquation_format)
					{
						parseValueFromParam(param, &value);
					}
					else
					{
						if (i + 1 != argc) value = &(argv[i + 1][0]);
						else value = NULL;
					}
					string par = string(param);
					if (m_mapsCparamParam_info.find(par) != m_mapsCparamParam_info.end())
					{
						if (m_mapsCparamParam_info.at(par).hasValue())
						{
							if (value == NULL) throw OptionsException(string("Missing value of parameter '--") + par + string("'!"));

							ret = m_map_ssEntered_param_value.insert(pair<string, string>(par, string(value)));
							if (!ret.second) throw OptionsException(string("Redefining parameter '--") + par + string("'!"));
							if (!m_bEquation_format) ++i;
						}
						else
						{
							ret = m_map_ssEntered_param_value.insert(pair<string, string>(par, string("")));
							if (!ret.second) throw OptionsException(string("Redefining parameter '--") + par + string("'!"));
						}
					}
					else
					{
						throw OptionsException(string("Invalid parameter '--") + par + string("'!"));
					}
				}
				else if (argv[i][1] != '-')
				{
					string param = string(&(argv[i][1]));
					map<string, string>::iterator it = m_mapssShort_to_long_params.find(param);
					if (it != m_mapssShort_to_long_params.end())
					{
						if (m_mapsCparamParam_info.at(it->second).hasValue())
						{
							if (i + 1 != argc)
							{
								ret = m_map_ssEntered_param_value.insert(pair<string, string>(it->second, string(argv[i + 1])));
								if (!ret.second) throw OptionsException(string("Redefining parameter '--") + param + string("'!"));
								++i;
							}
							else
							{
								throw OptionsException(string("Missing value of parameter '-") + param + string("'!"));
							}
						}
						else
						{
							ret = m_map_ssEntered_param_value.insert(pair<string, string>(it->second, string("")));
							if (!ret.second) throw OptionsException(string("Redefining parameter '-") + param + string("'!"));
						}
					}
					else
					{
						throw OptionsException(string("Invalid parameter '-") + param + string("'!")); //exception: param doesn't exists
					}
				}
				else
				{
					throw OptionsException(string("Invalid parameter '") + string(argv[i]) + string("'!")); // exception: param must have name 
				}
			}
			else
			{
				throw OptionsException(string("Invalid parameter '") + string(argv[i]) + string("'!")); //exception: params must start with '-'
			}
		}
		else
		{
			throw OptionsException(string("Invalid parameter '") + string(argv[i]) + string("'!")); //exception: param of length less than 2 cannot exists
		}
	}

	checkRequiredParams();
}

/// <summary>
/// Set description of program for help.
/// </summary>
/// <param name="description">The description of program.</param>
void ParseArguments::setDescription(std::string description)
{
	m_sDescription.assign(description);
}

/// <summary>
/// Set program name for usage -- usage program_name: [params].
/// </summary>
/// <param name="program_name">The program_name.</param>
void ParseArguments::setProgramName(std::string program_name)
{
	m_sProgram_name.assign(program_name);
}

/// <summary>
/// Function for checking for all required parameters was set.
/// </summary>
void ParseArguments::checkRequiredParams()
{
	for (map<string, Param>::iterator it = m_mapsCparamParam_info.begin(); it != m_mapsCparamParam_info.end(); ++it)
	{
		if (!it->second.isRequired()) continue;
		if (m_map_ssEntered_param_value.find(it->first) != m_map_ssEntered_param_value.end())
		{
			continue;
		}
		else
		{
			throw OptionsException(string("Missing required argument --'") + it->first + string("'!")); //exception: param of length less than 2 cannot exists
		}
	}
}

/// <summary>
/// Function for checking if parameter was set.
/// </summary>
/// <param name="parameter_name">Parameter name (like 'help').</param>
/// <returns>True if present</returns>
bool ParseArguments::checkIfSet(string parameter_name)
{
	return m_map_ssEntered_param_value.find(parameter_name) != m_map_ssEntered_param_value.end() ? true : false;
}

/// <summary>
/// Get the value of parameter, if parameter doesn't have a value then return "". If parameter doesn't be present, it return "".
/// </summary>
/// <param name="param">Parameter name (like 'help').</param>
/// <returns></returns>
string ParseArguments::getParamValue(string param)
{
	return checkIfSet(param) ? m_map_ssEntered_param_value.at(param) : "";
}