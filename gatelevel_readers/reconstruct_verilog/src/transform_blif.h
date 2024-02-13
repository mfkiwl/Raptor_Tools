#pragma once
/**
 * @file transforme_blif.h
 * @author Manadher Kharroubi (manadher@gmail.com)
 * @brief
 * @version 0.1
 * @date 2024-01-
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "reconstruct_dsp19.h"
#include "reconstruct_dsp38.h"
#include "reconstruct_ram18kx2.h"
#include "reconstruct_ram36k.h"

class Eblif_Transformer {
  std::vector<TDP_RAM18KX2_instance> TDP_RAM18KX2_instances;
  std::vector<TDP_RAM36K_instance> TDP_RAM36K_instances;
  std::vector<dsp38_instance> dsp38_instances;
  std::vector<dsp19_instance> dsp19_instances;
  std::unordered_map<std::string, std::string>
  blifToCPlusPlusMap(const std::vector<std::string> &tokens) {
    std::unordered_map<std::string, std::string> res;
    for (const std::string &token : tokens) {
      // Split each token using the '=' character
      size_t equalSignPos = token.find('=');
      if (equalSignPos != std::string::npos) {
        std::string key = token.substr(0, equalSignPos);
        std::string value = token.substr(equalSignPos + 1);

        // Add key-value pair to the result string
        res[key] = value;
      }
    }
    return res;
  }

  /**
   * @brief some useful functions
   *
   * transform(str.begin(), str.end(), str.begin(), ::toupper);
   *
   *
   */

  void replaceCarryTokens(std::string &line) {
    // Replace tokens
    size_t pos = 0;
    while ((pos = line.find(".cin", pos)) != std::string::npos) {
      line.replace(pos, 4, ".CIN");
      pos += 4;
    }
    pos = 0;
    while ((pos = line.find(".g", pos)) != std::string::npos) {
      line.replace(pos, 2, ".G");
      pos += 2;
    }
    pos = 0;
    while ((pos = line.find(".p", pos)) != std::string::npos) {
      line.replace(pos, 2, ".P");
      pos += 2;
    }
    pos = 0;
    while ((pos = line.find(".cout", pos)) != std::string::npos) {
      line.replace(pos, 5, ".COUT");
      pos += 5;
    }
    pos = 0;
    while ((pos = line.find(".sumout", pos)) != std::string::npos) {
      line.replace(pos, 7, ".O");
      pos += 7;
    }
  }

  bool is_binary_param_(const std::string &param) {
    /* Must be non-empty */
    if (param.empty()) {
      return false;
    }

    /* The string must contain only '0' and '1' */
    for (size_t i = 0; i < param.length(); ++i) {
      if (param[i] != '0' && param[i] != '1') {
        return false;
      }
    }

    /* This is a binary word param */
    return true;
  }
  bool is_real_param_(const std::string &param) {
    /* Must be non-empty */
    if (param.empty()) {
      return false;
    }

    /* The string must match the regular expression */
    const std::regex real_number_expr(
        "[+-]?([0-9]*\\.[0-9]+)|([0-9]+\\.[0-9]*)");
    if (!std::regex_match(param, real_number_expr)) {
      return false;
    }

    /* This is a real number param */
    return true;
  }
  unsigned long long veriValue(const string &exp) {
    if (!isdigit(exp[0]))
      throw(std::invalid_argument(
          "Not a valid expression (i.e 4'h0A9 ) at line " +
          std::to_string(__LINE__) + " " + exp));
    stringstream ss(exp);
    string dd;
    char *str, *stops;
    unsigned int aa;
    int bb = 2;
    ss >> aa >> dd;
    if (aa < 1 || aa > 64)
      throw(std::invalid_argument(
          "Not supporting more than 64 bit values or less than one bit " +
          exp));
    if (dd[1] == 'b' || dd[1] == 'B')
      bb = 2;
    else if (dd[1] == 'h' || dd[1] == 'H')
      bb = 16;
    else if (dd[1] == 'd' || dd[1] == 'D')
      bb = 10;
    else
      throw(std::invalid_argument("Invalid base indicator " + string(1, dd[1]) +
                                  " in " + exp +
                                  ", should be in {d,D,b,B,h,H}"));
    str = &dd[2];
    return strtoull(str, &stops, bb);
  }
  string bitsOfHexaDecimal(string &s) {
    if (!s.size()) {
      throw(std::invalid_argument(
          "Can't generate an hexadecimal number out of an empty string"));
    }
    for (auto &d : s) {
      if (d == 'z' || d == 'Z')
        d = 'x';
      if (!isxdigit(d) && d != 'x' && d != 'X')
        throw(std::invalid_argument("Non hexadigit in hexadecimal string" + s));
    }
    string res = "";
    res.reserve(4 * s.size());
    for (auto d : s) {
      for (auto c : hexDecoder[d])
        res.push_back(c);
    }
    return res;
  }
  int1024_t bigDecimalInteger(string &s) {
    if (!s.size()) {
      throw(std::invalid_argument(
          "Can't generate a decimal number out of an empty string"));
    }
    for (auto d : s) {
      if (!isdigit(d))
        throw(std::invalid_argument("Non digit in adecimal string" + s));
    }
    if (s.size() > mx.size() || (s.size() == mx.size() && s > mx)) {
      throw(std::invalid_argument(
          "Can't generate a decimal number bigger than " + mx));
    }
    int1024_t res = 0;
    for (auto d : s) {
      res = res * 10;
      res = res + (d - '0');
    }
    return res;
  }
  string bitsOfBigDecimal(string &s) {

    string res = "";
    int1024_t v = bigDecimalInteger(s);
    int1024_t r;
    while (v) {
      r = v % 2;
      res.push_back(r ? '1' : '0');
      v = v / 2;
    }
    reverse(begin(res), end(res));
    return res;
  }
  void bits(const string &exp, std::vector<std::string> &vec_, string &strRes) {
    std::vector<std::string> vec;
    if (exp.size() < 4)
      throw(std::invalid_argument(
          "Not a valid expression, should be of size more than 3 " + exp));
    if (!isdigit(exp[0]))
      throw(std::invalid_argument(
          "Not a valid expression (i.e 4'h0A9 ) at line " +
          std::to_string(__LINE__) + " " + exp));
    stringstream ss(exp);
    string rad_and_value;

    unsigned int bit_size;
    ss >> bit_size >> rad_and_value;
    string value = rad_and_value.substr(2);
    string bit_value;

    if (rad_and_value[1] == 'b' || rad_and_value[1] == 'B') {
      bit_value = value;
      for (auto &d : bit_value) {
        if (d == 'z' || d == 'Z')
          d = 'x';
        if ('1' != d && '0' != d && 'x' != d)
          throw(std::invalid_argument("Not valid bit value " + string(1, d) +
                                      " in " + exp));
      }
    } else if (rad_and_value[1] == 'h' || rad_and_value[1] == 'H') {
      bit_value = bitsOfHexaDecimal(value);
    } else if (rad_and_value[1] == 'd' || rad_and_value[1] == 'D') {
      bit_value = bitsOfBigDecimal(value);
    } else
      throw(std::invalid_argument("Invalid base indicator " +
                                  string(1, rad_and_value[1]) + " in " + exp +
                                  ", should be in {d,D,b,B,h,H}"));
    strRes = string(bit_size, '0');
    vec = vector<string>(bit_size, "0");

    for (unsigned idx = 0; idx < bit_size && idx < bit_value.size(); ++idx) {
      strRes[bit_size - 1 - idx] = bit_value[bit_value.size() - 1 - idx];
      if ('x' == bit_value[bit_value.size() - 1 - idx])
        vec[bit_size - 1 - idx] = "$undef";
      else
        vec[bit_size - 1 - idx] =
            std::string(1, bit_value[bit_value.size() - 1 - idx]);
    }
    // expand leading x s
    unsigned pos = 0;
    while (pos < vec.size() && "0" == vec[pos])
      ++pos;
    if (pos < vec.size() && "$undef" == vec[pos]) {
      for (unsigned idx = 0; idx < pos; ++idx) {
        vec[idx] = "$undef";
        strRes[idx] = 'x';
      }
    }
    for (unsigned vIdx = 0; vIdx < vec.size(); vIdx++) {
      vec_.push_back(vec[vIdx]);
    }
  }
  std::vector<unsigned> entryTruth(unsigned long long e, unsigned long long w) {
    std::vector<unsigned> res(w, 0);
    unsigned p = 1;
    for (unsigned int i = 0; i < w; ++i) {
      int k = e & p;
      if (k)
        res[w - 1 - i] = 1;
      p *= 2;
    }
    return res;
  }
  void simpleTruthTable(std::string tr, std::string w,
                        std::vector<std::vector<unsigned>> &vec) {
    if (is_binary_param_(w))
      w = "32'b" + w;
    if (is_binary_param_(tr))
      tr = "512'b" + tr;
    unsigned long long width = veriValue(w);
    string stringRes;
    vector<string> v;
    bits(tr, v, stringRes);

    for (unsigned i = 0; i < stringRes.size(); ++i) {
      if ('1' == stringRes[stringRes.size() - 1 - i]) {
        auto ent = entryTruth(i, width);
        ent.push_back(1);
        vec.push_back(ent);
      }
    }
  }
  void vector_print(std::ostream &f, vector<unsigned> &vec) {
    for (auto n : vec) {
      f << " " << n;
    }
    f << std::endl;
  }
  struct names_str {
    std::string Y = "";
    std::vector<std::string> A;
    std::string INIT_VALUE;
    std::string W;
    std::vector<std::vector<unsigned>> truth_table;
    void print_names(std::ostream &f, Eblif_Transformer *t) {
      t->simpleTruthTable(INIT_VALUE, "32'd" + W, truth_table);
      f << ".names"
        << " ";
      for (int i = A.size() - 1; i > -1; --i) {
        f << A[i] << " ";
      }
      f << Y << std::endl;
      for (auto &v : truth_table) {
        t->vector_print(f, v);
      }
    }
  };
  // Latch flattening
  std::unordered_map<std::string, std::string> latch_lut_LUT_strs = {
      {"LATCH", "10101100"},
      {"LATCHN", "10101100"},
      {"LATCHR", "0000101000001100"},
      {"LATCHS", "1111101011111100"},
      {"LATCHNR", "0000101000001100"},
      {"LATCHNS", "1111101011111100"},
      {"LATCHSRE",
       "1011101111110011111100111111001100000000000000000000000000000000"},
      {"LATCHNSRE",
       "1111001110111011111100111111001100000000000000000000000000000000"}};
  std::unordered_map<std::string, std::string> latch_lut_WIDTH_strs = {
      {"LATCH", "3"},    {"LATCHN", "3"},   {"LATCHR", "4"},
      {"LATCHS", "4"},   {"LATCHNR", "4"},  {"LATCHNS", "4"},
      {"LATCHSRE", "6"}, {"LATCHNSRE", "6"}};
  std::unordered_map<std::string, std::string> latch_ports = {
      {"LATCH", "DGQ"},       {"LATCHN", "DGQ"},      {"LATCHR", "DGRQ"},
      {"LATCHS", "DGRQ"},     {"LATCHNR", "DGRQ"},    {"LATCHNS", "DGRQ"},
      {"LATCHSRE", "QSRDGE"}, {"LATCHNSRE", "QSRDGE"}};
  std::unordered_map<std::string, std::string> lut_A_port_connections = {
      {"LATCH", "GQD"},       {"LATCHN", "GDQ"},      {"LATCHR", "GRQD"},
      {"LATCHS", "GRQD"},     {"LATCHNR", "GRDQ"},    {"LATCHNS", "GRDQ"},
      {"LATCHSRE", "RGEQSD"}, {"LATCHNSRE", "REGQSD"}};
  std::unordered_map<std::string, std::string> lut_port_map_LATCH{
      {"G", "A[2]"}, {"Q", "A[1]"}, {"D", "A[0]"}};
  std::unordered_map<std::string, std::string> lut_port_map_LATCHN{
      {"G", "A[2]"}, {"D", "A[1]"}, {"Q", "A[0]"}};
  std::unordered_map<std::string, std::string> lut_port_map_LATCHR{
      {"G", "A[3]"}, {"R", "A[2]"}, {"Q", "A[1]"}, {"D", "A[0]"}};
  std::unordered_map<std::string, std::string> lut_port_map_LATCHS{
      {"G", "A[3]"}, {"R", "A[2]"}, {"Q", "A[1]"}, {"D", "A[0]"}};
  std::unordered_map<std::string, std::string> lut_port_map_LATCHNR{
      {"G", "A[3]"}, {"R", "A[2]"}, {"D", "A[1]"}, {"Q", "A[0]"}};
  std::unordered_map<std::string, std::string> lut_port_map_LATCHNS{
      {"G", "A[3]"}, {"R", "A[2]"}, {"D", "A[1]"}, {"Q", "A[0]"}};
  std::unordered_map<std::string, std::string> lut_port_map_LATCHSRE{
      {"R", "A[5]"}, {"G", "A[4]"}, {"E", "A[3]"},
      {"Q", "A[2]"}, {"S", "A[1]"}, {"D", "A[0]"}};
  std::unordered_map<std::string, std::string> lut_port_map_LATCHNSRE{
      {"R", "A[5]"}, {"E", "A[4]"}, {"G", "A[3]"},
      {"Q", "A[2]"}, {"S", "A[1]"}, {"D", "A[0]"}};
  std::unordered_map<std::string, std::unordered_map<std::string, std::string>>
      latch_lut_port_conversion{{"LATCH", lut_port_map_LATCH},
                                {"LATCHN", lut_port_map_LATCHN},
                                {"LATCHR", lut_port_map_LATCHR},
                                {"LATCHS", lut_port_map_LATCHS},
                                {"LATCHNR", lut_port_map_LATCHNR},
                                {"LATCHNS", lut_port_map_LATCHNS},
                                {"LATCHSRE", lut_port_map_LATCHSRE},
                                {"LATCHNSRE", lut_port_map_LATCHNSRE}};
  string mx = "179769313486231590772930519078902473361797697894230657273"
              "430081157732675805500963132708477322407536021120113879871"
              "393357658789768814416622492847430639474124377767893424865"
              "485276302219601246094119453082952085005768838150682342462"
              "881473913110540827237163350510684586298239947245938479716"
              "304835356329624224137215";
  std::map<char, string> hexDecoder = {
      {'0', "0000"}, {'1', "0001"}, {'2', "0010"}, {'3', "0011"}, {'4', "0100"},
      {'5', "0101"}, {'6', "0110"}, {'7', "0111"}, {'8', "1000"}, {'9', "1001"},
      {'A', "1010"}, {'B', "1011"}, {'C', "1100"}, {'D', "1101"}, {'E', "1110"},
      {'F', "1111"}, {'X', "xxxx"}, {'a', "1010"}, {'b', "1011"}, {'c', "1100"},
      {'d', "1101"}, {'e', "1110"}, {'f', "1111"}, {'x', "xxxx"}};
  char hexEncode(std::string val) {
    static std::unordered_map<string, char> hexEncoder = {
        {"0000", '0'}, {"0001", '1'}, {"0010", '2'}, {"0011", '3'},
        {"0100", '4'}, {"0101", '5'}, {"0110", '6'}, {"0111", '7'},
        {"1000", '8'}, {"1001", '9'}, {"1010", 'A'}, {"1011", 'B'},
        {"1100", 'C'}, {"1101", 'D'}, {"1110", 'E'}, {"1111", 'F'}};
    if (hexEncoder.find(val) == end(hexEncoder))
      return 'X';
    return hexEncoder[val];
  }
  std::unordered_map<std::string, std::string> verilog_dsp_int_names = {
      {"RS_DSP_MULT",
       "    DSP38 #(\n        .DSP_MODE(\"MULTIPLY\"),\n        "
       ".OUTPUT_REG_EN(\"FALSE\"),\n        .INPUT_REG_EN(\"FALSE\")"},
      {"RS_DSP_MULT_REGIN",
       "    DSP38 #(\n        .DSP_MODE(\"MULTIPLY\"),\n        "
       ".OUTPUT_REG_EN(\"FALSE\"),\n        .INPUT_REG_EN(\"TRUE\")"},
      {"RS_DSP_MULT_REGOUT",
       "    DSP38 #(\n        .DSP_MODE(\"MULTIPLY\"),\n        "
       ".OUTPUT_REG_EN(\"TRUE\"),\n        .INPUT_REG_EN(\"FALSE\")"},
      {"RS_DSP_MULT_REGIN_REGOUT",
       "    DSP38 #(\n        .DSP_MODE(\"MULTIPLY\"),\n        "
       ".OUTPUT_REG_EN(\"TRUE\"),\n        .INPUT_REG_EN(\"TRUE\")"},
      {"RS_DSP_MULTADD",
       "    DSP38 #(\n        .DSP_MODE(\"MULTIPLY_ADD_SUB\"),\n        "
       ".OUTPUT_REG_EN(\"FALSE\"),\n        .INPUT_REG_EN(\"FALSE\")"},
      {"RS_DSP_MULTADD_REGIN",
       "    DSP38 #(\n        .DSP_MODE(\"MULTIPLY_ADD_SUB\"),\n        "
       ".OUTPUT_REG_EN(\"FALSE\"),\n        .INPUT_REG_EN(\"TRUE\")"},
      {"RS_DSP_MULTADD_REGOUT",
       "    DSP38 #(\n        .DSP_MODE(\"MULTIPLY_ADD_SUB\"),\n        "
       ".OUTPUT_REG_EN(\"TRUE\"),\n        .INPUT_REG_EN(\"FALSE\")"},
      {"RS_DSP_MULTADD_REGIN_REGOUT",
       "    DSP38 #(\n        .DSP_MODE(\"MULTIPLY_ADD_SUB\"),\n        "
       ".OUTPUT_REG_EN(\"TRUE\"),\n        .INPUT_REG_EN(\"TRUE\")"},
      {"RS_DSP_MULTACC",
       "    DSP38 #(\n        .DSP_MODE(\"MULTIPLY_ACCUMULATE\"),\n        "
       ".OUTPUT_REG_EN(\"FALSE\"),\n        .INPUT_REG_EN(\"FALSE\")"},
      {"RS_DSP_MULTACC_REGIN",
       "    DSP38 #(\n        .DSP_MODE(\"MULTIPLY_ACCUMULATE\"),\n        "
       ".OUTPUT_REG_EN(\"FALSE\"),\n        .INPUT_REG_EN(\"TRUE\")"},
      {"RS_DSP_MULTACC_REGOUT",
       "    DSP38 #(\n        .DSP_MODE(\"MULTIPLY_ACCUMULATE\"),\n        "
       ".OUTPUT_REG_EN(\"TRUE\"),\n        .INPUT_REG_EN(\"FALSE\")"},
      {"RS_DSP_MULTACC_REGIN_REGOUT",
       "    DSP38 #(\n        .DSP_MODE(\"MULTIPLY_ACCUMULATE\"),\n        "
       ".OUTPUT_REG_EN(\"TRUE\"),\n        .INPUT_REG_EN(\"TRUE\")"}};

public:
  void rs_transform_eblif(std::istream &ifs, std::ostream &ofs) {
    // port_case_compare();
    // return;
    std::string last_ckt = "";
    std::string line;
    while (std::getline(ifs, line)) {
      std::string ln(line);
      while ('\\' == ln.back() && std::getline(ifs, line)) {
        ln.pop_back();
        ln += " " + line;
      }
      auto tokens = split_on_space(ln);
      if (!tokens.size()) {
        ofs << line << "\n";
        continue;
      }
      if (tokens.size() < 4) {
        if (tokens.size() == 3 && tokens[0] == ".param") {
          std::string par_name = tokens[1];
          if (par_name.back() == '"')
            par_name.pop_back();
          if (par_name.size() && par_name[0] == '"') {
            for (size_t idx = 0; idx < par_name.size() - 1; ++idx)
              par_name[idx] = par_name[idx + 1];
            par_name.pop_back();
          }
          std::string par_value = tokens[2];
          if (par_value.back() == '"')
            par_value.pop_back();
          if (par_value.size() && par_value[0] == '"') {
            for (size_t idx = 0; idx < par_value.size() - 1; ++idx)
              par_value[idx] = par_value[idx + 1];
            par_value.pop_back();
          }
          if (last_ckt == "dsp38" && dsp38_instances.size() &&
              dsp38_instances.back().set_param(par_name, par_value)) {
          } else if (last_ckt == "dsp19" && dsp19_instances.size() &&
                     dsp19_instances.back().set_param(par_name, par_value)) {
          } else if (last_ckt == "tdp_ram18kx2" &&
                     TDP_RAM18KX2_instances.size() &&
                     TDP_RAM18KX2_instances.back().set_param(par_name,
                                                             par_value)) {
          } else if (last_ckt == "tdp_ram36k" && TDP_RAM36K_instances.size() &&
                     TDP_RAM36K_instances.back().set_param(par_name,
                                                           par_value)) {
          } else {
            ofs << ln << "\n";
          }
        } else if (tokens[0] == ".end") {
          unsigned int cnt = 0;
          for (auto &ds : dsp38_instances) {
            ds.print(ofs);
          }
          for (auto &ds : dsp19_instances) {
            ds.print(ofs);
          }
          for (auto &rm : TDP_RAM18KX2_instances) {
            cnt++;
            rm.print(ofs, cnt);
          }
          for (auto &rm : TDP_RAM36K_instances) {
            cnt++;
            rm.print(ofs, cnt);
          }

          ofs << ln << "\n";
        } else {
          ofs << ln << "\n";
        }
        continue;
      }
      if (tokens[0] == ".subckt") {
        last_ckt = "";
        std::string name = tokens[1];
        std::transform(name.begin(), name.end(), name.begin(), ::tolower);
        if (name.find("dff") != std::string::npos ||
            name == std::string("adder_carry") ||
            name == std::string("carry")) {
          if (name == std::string("carry")) {
            name = std::string("adder_carry");
            for (int i = 2; i < tokens.size(); ++i) {
              auto itr = tokens[i].find('=');
              if (std::string::npos == itr) {
                continue;
              }
              std::transform(tokens[i].begin(), tokens[i].begin() + itr,
                             tokens[i].begin(), ::tolower);
              if (tokens[i].size() > 2 && tokens[i][0] == 'o' &&
                  tokens[i][1] == '=') {
                tokens[i] = std::string("sumout=") + tokens[i].substr(2);
              }
            }
          }
          tokens[1] = name;
          for (auto &t : tokens) {
            ofs << t << " ";
          }
          ofs << "\n";
        } else if (name.find("lut") == 0) {
          bool supported = true;
          names_str names;
          names.W = name.substr(3);
          bool wholeA = false;
          // test if we have the supported format
          // .subckt LUTx A[0]=i_name0 A[1]=i_name1 ...  A[x-1]=i_name<x-1>
          // Y=o_name
          if (names.W.size() != 1 || !isdigit(names.W[0])) {
            supported = false;
          } else {
            for (uint idx = 2; idx < tokens.size() - 1; ++idx) {
              std::string s = tokens[idx];
              s[2] = '-';
              if (s.find("A[-]=") == std::string::npos &&
                  s.find("A=") == std::string::npos) {
                supported = false;
                break;
              }
              if (s.find("A=") != std::string::npos) {
                wholeA = true;
              }
            }
            if (tokens.back().find("Y=") != 0) {
              supported = false;
              break;
            }
          }
          if (!supported) {
            for (auto &t : tokens) {
              ofs << t << " ";
            }
            ofs << "\n";
            continue;
          }
          // Populate names and call print_names()

          names.Y = tokens.back().substr(2);
          names.A = std::vector<std::string>(tokens.size() - 3, "$undef");
          if (wholeA) {
            names.A[0] = tokens[2].substr(2);
          } else {
            for (int idx = 2; idx < tokens.size() - 1; ++idx) {
              int pos = tokens[idx][2] - '0';
              names.A[pos] = tokens[idx].substr(5);
            }
          }
          std::string init_line;
          std::getline(ifs, init_line);
          auto init_tokens = split_on_space(init_line);
          while (!init_tokens.size()) {
            std::getline(ifs, init_line);
            init_tokens = split_on_space(init_line);
          }
          if (init_tokens.size() < 3 || init_tokens[0] != ".param" ||
              init_tokens[1] != "INIT_VALUE" ||
              !is_binary_param_(init_tokens[2])) {
            //
            for (auto &t : tokens) {
              ofs << t << " ";
            }
            ofs << "\n";
            for (auto &t : init_tokens) {
              ofs << t << " ";
            }
            ofs << "\n";
            continue;
          }
          names.INIT_VALUE = init_tokens[2];
          names.print_names(ofs, this);
        } else if (name.find("latch") == 0) {
          try {
            // Implement a latch using a lut (if possible : well written from
            // rs yosys)
            if (end(latch_lut_WIDTH_strs) ==
                latch_lut_WIDTH_strs.find(tokens[1])) {
              ofs << ln << "\n";
              continue;
            }
            names_str names;
            names.W = latch_lut_WIDTH_strs[tokens[1]];
            uint sz = names.W[0] - '0';
            names.A = std::vector<std::string>(sz, "$undef");
            for (uint i = 2; i < tokens.size(); ++i) {
              std::string s = std::string(1, tokens[i][0]);
              std::string port_conn = std::string(tokens[i]).substr(2);
              if (s == "Q")
                names.Y = port_conn; // output of the latch
              uint idx = latch_lut_port_conversion.at(tokens[1]).at(s)[2] - '0';
              int w_pos = -1;
              w_pos = idx;

              names.A.at(w_pos) = port_conn;
            }
            names.INIT_VALUE = latch_lut_LUT_strs.at(tokens[1]);
            names.print_names(ofs, this);
          } catch (...) {
            for (auto &t : tokens) {
              ofs << t << " ";
            }
            ofs << "\n";
          }
        } else if (name.find("dsp38") == 0) {
          last_ckt = "dsp38";
          dsp38_instances.push_back(dsp38_instance());
          dsp38_instances.back().port_connections = blifToCPlusPlusMap(tokens);
        } else if (name.find("dsp19") == 0) {
          last_ckt = "dsp19";
          dsp19_instances.push_back(dsp19_instance());
          dsp19_instances.back().port_connections = blifToCPlusPlusMap(tokens);
        } else if (name.find("tdp_ram18kx2") == 0) {
          last_ckt = "tdp_ram18kx2";
          TDP_RAM18KX2_instances.push_back(TDP_RAM18KX2_instance());
          TDP_RAM18KX2_instances.back().port_connections =
              blifToCPlusPlusMap(tokens);
        } else if (name.find("tdp_ram36k") == 0) {
          last_ckt = "tdp_ram36k";
          TDP_RAM36K_instances.push_back(TDP_RAM36K_instance());
          TDP_RAM36K_instances.back().port_connections =
              blifToCPlusPlusMap(tokens);
        } else {
          for (auto &t : tokens) {
            ofs << t << " ";
          }
          ofs << "\n";
        }
      } else {
        ofs << line << "\n";
      }
    }
  }
  void rs_transform_verilog(std::istream &ifs, std::ostream &ofs) {
    std::string line;
    bool within_rs_dsp = false;
    bool is_carry_mod = false;
    std::string hdr = "";
    while (std::getline(ifs, line)) {
      std::string ln(line);
      while (!ln.empty() && '\\' == ln.back() && std::getline(ifs, line)) {
        ln.pop_back();
        ln += " " + line;
      }
      auto tokens = split_on_space(ln);
      if (!tokens.size()) {
        // If failed to tokenize, Write the line as is and get the next line
        ofs << line << "\n";
        continue;
      }
      if (tokens[0].find("INIT_i") != std::string::npos) {
        auto pos = tokens[0].find("'b");
        if (pos != std::string::npos) {
          std::string mval = tokens[0].substr(pos + 2);
          std::string bck = "";
          while (mval.back() != '0' && mval.back() != '1') {
            bck.push_back(mval.back());
            mval.pop_back();
          }
          std::reverse(bck.begin(), bck.end());
          if (mval.size() % 4) { // not divisible by 4
            ofs << line << "\n";
            continue;
          }
          ofs << "        " <<tokens[0].substr(0, pos) << "'h";
          for (int i = 0; i < mval.size(); i += 4) {
            ofs << hexEncode(mval.substr(i, 4));
          }
          ofs << bck << "\n";
          continue;
        } else {
          ofs << line << "\n";
          continue;
        }
      } else if (verilog_dsp_int_names.find(tokens[0]) !=
                 end(verilog_dsp_int_names)) {
        within_rs_dsp = true;
        hdr = verilog_dsp_int_names[tokens[0]];
      } else if (tokens[0] == "ADDER_CARRY") {
        within_rs_dsp = false;
        is_carry_mod = true;
        ofs << "    CARRY #(\n";
      } else if (tokens[0] == ");") {
        within_rs_dsp = false;
        is_carry_mod = false;
        hdr = "";
        ofs << line << "\n";
      } else if (is_carry_mod && '.' == tokens[0][0]) {
        replaceCarryTokens(line);
        ofs << line << '\n';
      } else if (within_rs_dsp && '.' == tokens[0][0]) {
        if (tokens[0].find("MODE_BITS") == 1) {
          std::string bts = tokens[0].substr(15);
          bts.pop_back();
          if (bts.size() != 80) {
            std::invalid_argument(
                "Invalid MODE_BITS, expected exactly 80 bits!");
          }
          std::string COEFF_0 = bts.substr(0, 20);
          std::string COEFF_1 = bts.substr(20, 20);
          std::string COEFF_2 = bts.substr(40, 20);
          std::string COEFF_3 = bts.substr(60, 20);
          std::string print_Coef = ",\n        .COEFF_0(20'b" + COEFF_0 +
                                   "),\n        .COEFF_1(20'b" + COEFF_1 +
                                   "),\n        .COEFF_2(20'b" + COEFF_2 +
                                   "),\n        .COEFF_3(20'b" + COEFF_3 + ")";
          hdr += print_Coef;
          ofs << hdr << std::endl;
          continue;
        }
        auto par_pos = tokens[0].find('(');
        // Error if not found
        transform(tokens[0].begin(), tokens[0].begin() + par_pos,
                  tokens[0].begin(), ::toupper);

        ofs << "        ";
        if (tokens[0].find(std::string(".LRESET")) == 0) {
          ofs << ".RESET" << tokens[0].substr(par_pos);
          for (int idx = 1; idx < tokens.size(); ++idx) {
            ofs << " " << tokens[idx];
          }
        } else if (tokens[0].find(std::string(".SATURATE_ENABLE")) == 0) {
          ofs << ".SATURATE" << tokens[0].substr(par_pos);
          for (int idx = 1; idx < tokens.size(); ++idx) {
            ofs << " " << tokens[idx];
          }
        } else
          for (auto &a : tokens) {
            ofs << a << " ";
          }
        ofs << "\n";
      } else {
        ofs << line << "\n";
      }
    }
  }
  void transform_files(std::string in_file, std::string out_file,
                       bool isblif = true) {

    std::ifstream ifs(in_file);
    std::ofstream ofs(out_file);
    if (isblif) {
      rs_transform_eblif(ifs, ofs);
    } else {
      rs_transform_verilog(ifs, ofs);
    }
    ifs.close();
    ofs.close();
  }
  void printFileContents(FILE *pf, FILE *pfout = nullptr, bool close = false) {
    if (!pf) {
      std::cerr << "Invalid file pointer!" << std::endl;
      return;
    }

    char buffer[1024];
    size_t bytesRead = 0;
    if (pfout) {
      while ((bytesRead = fread(buffer, 1, sizeof(buffer), pf)) > 0) {
        fwrite(buffer, 1, bytesRead, pfout);
      }
    } else {
      while ((bytesRead = fread(buffer, 1, sizeof(buffer), pf)) > 0) {
        fwrite(buffer, 1, bytesRead, stdout);
      }
    }

    if (ferror(pf)) {
      std::cerr << "Error reading the file!" << std::endl;
    }
    if (close)
      fclose(pf); // Close the memory stream.
  }
  inline FILE *open_and_transform_eblif(const char *filename) {
    // FILE* infile = std::fopen(filename, "r");
    // int fd = fileno(infile);
    std::ifstream fstr;
    fstr.open(filename);
    std::stringstream ss;
    ss << "";
    // Call transform function
    rs_transform_eblif(fstr, ss);
    std::string data = ss.str();
    FILE *infile = fmemopen((void *)data.c_str(), data.size(), "r");
    printFileContents(infile);
    return infile;
  }
};
