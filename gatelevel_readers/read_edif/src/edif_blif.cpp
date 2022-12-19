#include "edif_blif.hpp"

struct inst {
  std::string name_;
  std::string mod_name_;
  std::vector<std::pair<std::string, std::string>> conns_;
  std::unordered_map<std::string, std::string> params_;
  std::vector<std::vector<unsigned>> truthTable_;
  inst() {}
  inst(std::string name, std::string mod_name,
       std::vector<std::pair<std::string, std::string>> conn)
      : name_(name), mod_name_(mod_name), conns_(conn) {}
  void p_print(std::ostream &f) {

    f << "Instance : " << name_ << " of module " << mod_name_ << std::endl;
    for (auto &p : conns_) {
      f << "." << p.first << "(" << p.second << "), ";
    }
    f << std::endl;
    for (auto &p : params_) {
      f << p.first << " : " << p.second << endl;
    }

    if ("$lut" == mod_name_.substr(0, 4)) {
      blif_names_print(f);
    }
  }
  void blif_names_print(std::ostream &f) {
    f << ".names";
    for (int idx = conns_.size() - 2; idx >= 0; --idx) {
      string s =
          (conns_[idx].second == "GND_NET") ? "$false" : conns_[idx].second;
      s = (s == "VCC_NET") ? "$true" : s;
      s = (s == "n3") ? "$undef" : s;
      if ('\\' == s[0])
        s[0] = ' ';
      f << " " << s;
    }
    f << " " << conns_.back().second << std::endl;
    for (auto &v : truthTable_) {
      for (int i = 0; i < v.size() - 1; ++i) {
        f << v[i];
      }
      f << " " << v[v.size() - 1] << endl;
    }
  }
  void blif_subckt_print(std::ostream &f) {
    string no_param_name;
    // reducing a correctly named parametrized module MyModule(par1=99) to
    // MyModule discuss with thierry !
    for (auto k : mod_name_)
      if ('(' == k)
        break;
      else
        no_param_name.push_back(k);
    f << ".subckt " << no_param_name;
    for (auto &p : conns_) {
      string s = (p.second == "GND_NET") ? "$false" : p.second;
      s = (s == "VCC_NET") ? "$true" : s;
      s = (s == "n3") ? "$undef" : s;
      f << " " << p.first << "=" << s;
    }
    f << std::endl;
    for (auto &p : params_) {
      // TO_DO verify that verific gives always binary otherwise hande other
      // formats
      std::string s(p.second);
      std::size_t found = s.find('b');
      if (found == std::string::npos)
        found = s.find('B');
      if (found != std::string::npos) {
        for (int i = found; i >= 0; i--)
          s[i] = ' ';
      }
      while (s.size() && (s.back() == '"' || isspace(s.back())))
        s.pop_back();
      f << ".param " << p.first << " " << s << std::endl;
    }
  }
  void blif_print(std::ostream &f) {
    if ("$lut" == mod_name_.substr(0, 4)) {
      blif_names_print(f);
    } else {
      blif_subckt_print(f);
    }
  }
};

struct simple_netlist {
  void p_print(std::ostream &f) {
    f << "Netlist Name ----: " << name << std::endl;
    f << "Netlist in ports ----: " << std::endl;
    p_print(in_ports, f);
    f << "Netlist out ports ----: " << std::endl;
    p_print(out_ports, f);
    f << "Netlist nets ----: " << std::endl;
    p_print(nets, f);
    f << "Netlist blocks ----: " << std::endl;
    p_print(blocks, f);
  }
  void b_port_print_json(std::ostream &f) {
    bool first = true;
    f << "{\n\t\"inputs\": [";
    for (auto &in : in_ports) {
      if ('\\' == in[0])
        in[0] = ' ';
      if (!first)
        f << ",";
      f << "\n\t\t\"" << in << "\"";
      first = false;
    }
    f << "\n\t]," << endl;
    first = true;
    f << "\n\t\"outputs\": [";
    for (auto &in : out_ports) {
      if ('\\' == in[0])
        in[0] = ' ';
      if (!first)
        f << ",";
      f << "\n\t\t\"" << in << "\"";
      first = false;
    }
    f << "\n\t]\n}" << endl;
  }

  void b_print(std::ostream &f) {
    f << ".model " << name << std::endl;
    f << ".inputs";
    for (auto &in : in_ports) {
      if ('\\' == in[0])
        in[0] = ' ';
      f << " " << in;
    }
    f << endl;
    f << ".outputs";
    for (auto &in : out_ports) {
      if ('\\' == in[0])
        in[0] = ' ';
      f << " " << in;
    }
    f << endl;
    f << ".names $false" << endl;
    f << ".names $true\n1" << endl;
    f << ".names $undef" << endl;
    b_print(blocks, f);
    f << ".end" << endl;
  }
  void b_print(std::vector<inst> v, std::ostream &f) {
    for (auto &el : v) {
      el.blif_print(f);
    }
  }
  void p_print(std::vector<std::string> v, std::ostream &f) {
    for (auto &el : v) {
      f << "\t" << el;
      f << std::endl;
    }
    f << std::endl;
  }
  void p_print(std::vector<inst> v, std::ostream &f) {
    for (auto &el : v) {
      f << "\t";
      el.p_print(f);
      f << std::endl;
    }
    f << std::endl;
  }
  std::string name;
  std::vector<std::string> in_ports;
  std::vector<std::string> out_ports;
  std::vector<std::string> inout_ports;
  std::vector<std::string> nets;
  std::vector<inst> blocks;
};

void get_truth_table(std::string tt_output_str, int width, bool is_hex,
                     std::vector<std::vector<unsigned>> &tt_vector) {

  std::vector<std::vector<int>> rows;
  // std::vector<std::vector<unsigned> > tt_vector;
  std::string property_lut = tt_output_str;
  unsigned long long int index;

  if (is_hex) {
    int pos = property_lut.find("h");

    property_lut = property_lut.substr(pos + 1);
    index = std::stoul(property_lut, nullptr, 16);
  } else {
    index = std::stoll(property_lut);
  }
  int property_wid = width;
  int pp_wid = property_wid;
  int max_tt = 1;
  while (pp_wid != 0) {
    max_tt = (max_tt * 2);
    pp_wid--;
  }
  std::vector<int> property_lut_binary(max_tt, 0);
  int pp_required = max_tt - 1;
  while (index != 0) {
    property_lut_binary.at(pp_required) = (index % 2);
    index /= 2;
    pp_required--;
  }
  int tt_output = max_tt - 1;
  for (int tt = 0; tt < max_tt; tt++) {
    std::vector<unsigned> row_((property_wid + 1), 0);
    int tt_size = property_wid;
    row_.at(tt_size) = property_lut_binary.at(tt_output);
    tt_output--;
    int tt_i = tt;
    while (tt_i > 0) {
      int tt_input = tt_i % 2;
      tt_i = tt_i / 2;
      tt_size--;
      row_.at(tt_size) = tt_input;
    }
    if ((row_[row_.size() - 1]) == 1) {
      tt_vector.push_back(row_);
    }
  }
}

void ports_addition(std::vector<std::string> &ports, std::string port_name,
                    int port_size) {

  if ((port_size == 0) || (port_size == 1)) {
    ports.push_back(port_name);
  } else {
    for (auto s = 0; s < port_size; s++) {
      ports.push_back(port_name + "[" + to_string(s) + "]");
    }
  }
}

void seperate_ports(std::vector<std::tuple<
                        /*Port name original  */ std::string,
                        /*Port name renamed   */ std::string,
                        /*Port direction      */ std::string,
                        /*Port size           */ std::string>> &ports_vector,
                    std::vector<std::string> &in_ports,
                    std::vector<std::string> &out_ports,
                    std::vector<std::string> &inout_ports, bool is_lut,
                    std::string width) {

  for (unsigned int i = 0; i < ports_vector.size(); i++) {
    int size;
    if (string_compare(
            std::get<3>(ports_vector[i]),
            "")) // if the input port size is not defined set it to zero
      size = 0;
    else
      size = stoi(
          std::get<3>(ports_vector[i])); // else get the size from the value

    if (string_compare("INPUT", get<2>(ports_vector[i]))) {
      if (is_lut) {
        std::string empty;
        if (string_compare(width, empty)) {
          size = 0;
        } else {
          size = stoi(width);
        }
      }
      
      ports_addition(in_ports, std::get<0>(ports_vector[i]), size);
    } else if (string_compare("OUTPUT", get<2>(ports_vector[i]))) {
      ports_addition(out_ports, std::get<0>(ports_vector[i]), size);
    } else if (string_compare("INOUT", get<2>(ports_vector[i]))) {
      ports_addition(inout_ports, std::get<0>(ports_vector[i]), size);
    }
  }
}

std::string find_corresp_net(
    std::map<std::string, std::vector<std::tuple<

                              /*net_name           */ std::string,
                              /* net instance ref  */ std::string,
                              /* net name  renamed  */ std::string>>>
        &net_reduced_map,
    std::string ins_ref, std::string port_name) {
  std::pair<std::string, std::string> port_ref_pair =
      std::make_pair(port_name, ins_ref);
  std::pair<std::string, std::string> port_init_pair;
  std::vector<std::tuple<

      /*net_name           */ std::string,
      /* net instance ref  */ std::string,
      /* net name  renamed  */ std::string>>::iterator t;
  for (auto it = net_reduced_map.begin(); it != net_reduced_map.end(); it++) {
    for (long unsigned int i = 0; i < it->second.size(); i++) {
      port_init_pair = std::make_pair(std::get<0>(it->second[i]),
                                      std::get<1>(it->second[i]));
      if (port_init_pair == port_ref_pair) {
        t = (it->second.begin() + i);
        // port_init_pair= *t;
        // std::cout << "The deleted pair is "<< port_init_pair.first<< " "
        // <<port_init_pair.second<<std::endl;
        it->second.erase(t);

        return it->first;
      }
    }
  }
  return "";
}

void edif_blif(const char *InputFile, FILE *edif_bl) {

 std::vector<std::tuple<
    /*net_name           */ std::string,
    /* net instance ref  */ std::string,
    /* net name renamed  */ std::string>>
    net_reduced_vector;
std::map<std::string, std::vector<std::tuple<

                          /*net_name           */ std::string,
                          /* net instance ref  */ std::string,
                          /* net name renamed  */ std::string>>>
    net_reduced_map;
  pprint p1;
  cells_sep cell1_;

  FILE *fp;
  fp = fopen(InputFile, "r");
  // Read the file into a tree
  struct SNode *node = snode_parse(fp);
  cell1_.iterate(node);
  // Close the file stream
  fclose(fp);
  simple_netlist sn;

  std::stringstream ss;
  sn.name = cell1_.top_module;
  
  for (unsigned int itv = 0; itv < cell1_.cells_vector.size(); itv++) {
    // Initially create a net vector in which only the port name and its
    // corresponding net is present.
    for (auto it = cell1_.cells_vector[itv].net_map.begin();
         it != cell1_.cells_vector[itv].net_map.end(); it++) {

      for (long unsigned int i = 0; i < it->second.size(); i++) {

        std::string port_name = std::get<0>(it->second[i]);

        if (!string_compare(std::get<1>(it->second[i]),
                            "")) // condition check if member exist
        {
          if (string_compare(
                  std::get<2>(it->second[i]),
                  "")) // condition check if the port is from top module
          {
            for (auto itp = 0;
                 itp < cell1_.cells_vector[itv].ports_vector.size(); itp++) {
              if (string_compare(
                      std::get<1>(cell1_.cells_vector[itv].ports_vector[itp]),
                      std::get<0>(it->second[i]))) // comparing the port names
              {
                int port_size =
                    stoi(std::get<3>(
                        cell1_.cells_vector[itv].ports_vector[itp])) -
                    1;
                int pin_number = stoi(std::get<1>(it->second[i]));
                pin_number = port_size - pin_number;
                port_name = port_name + "[" + std::to_string(pin_number) + "]";
              }
            }
          } else {
            port_name = port_name + "[" + std::get<1>(it->second[i]) + "]";
          }
        }
        net_reduced_vector.push_back(std::make_tuple(
            port_name, std::get<2>(it->second[i]), std::get<3>(it->second[i])));
      }
      net_reduced_map.insert({it->first, net_reduced_vector});
      net_reduced_vector.clear();
    }

    // find the top cell which will be build for the blif
    if (string_compare(cell1_.cells_vector[itv].cell_name_orig,
                       cell1_.top_module)) {
      // get the cell ios and place it in the simple netlist vector
      seperate_ports(cell1_.cells_vector[itv].ports_vector, sn.in_ports,
                     sn.out_ports, sn.inout_ports, false, "");

      cell1_.instance_vector =
          cell1_.cells_vector[itv]
              .instance_vector; // use it directly remove it at the end
      //  add up all the instances
      for (unsigned int iti = 0; iti < cell1_.instance_vector.size(); iti++) {
        inst ins_;
        int input_port_size;
        ins_.name_ =
            std::get<1>(cell1_.instance_vector[iti]); // write the instance name
        ins_.mod_name_ = std::get<2>(
            cell1_.instance_vector[iti]); // write the instance model name
        if ((string_compare(std::get<2>(cell1_.instance_vector[iti]), "GND")) ||
            (string_compare(std::get<2>(cell1_.instance_vector[iti]), "VCC")))
          continue;
        for (unsigned int itvv = 0; itvv < cell1_.cells_vector.size(); itvv++) {
          std::vector<std::string> in_ports;
          std::vector<std::string> out_ports;
          std::vector<std::string> inout_ports;
          
          if (string_compare(cell1_.cells_vector[itvv].cell_name_renamed,
                             std::get<2>(cell1_.instance_vector[iti]))) {
            // Get all the ports of that instance cell
            seperate_ports(cell1_.cells_vector[itvv].ports_vector, in_ports,
                           out_ports, inout_ports,
                           std::get<5>(cell1_.instance_vector[iti]),
                           std::get<4>(cell1_.instance_vector[iti]));
            int inpt = 0;

            // Getting the net connections from the ports
            // First entering the inputs
            input_port_size = in_ports.size();
            for (inpt = in_ports.size() - 1; inpt >= 0; inpt--) {
              std::string result_net =
                  find_corresp_net(net_reduced_map, ins_.name_, in_ports[inpt]);
              if (!string_compare(result_net, ""))
                ins_.conns_.push_back(
                    std::make_pair(in_ports[inpt], result_net));
            }

            // First putting the output
            for (inpt = 0; inpt < out_ports.size(); inpt++) {
              std::string result_net = find_corresp_net(
                  net_reduced_map, ins_.name_, out_ports[inpt]);
              if (!string_compare(result_net, ""))
                ins_.conns_.push_back(
                    std::make_pair(out_ports[inpt], result_net));
            }
            for (inpt = 0; inpt < inout_ports.size(); inpt++) {
              std::string result_net = find_corresp_net(
                  net_reduced_map, ins_.name_, inout_ports[inpt]);
              if (!string_compare(result_net, ""))
                ins_.conns_.push_back(
                    std::make_pair(inout_ports[inpt], result_net));
            }

            break;
          }
        }
        if (std::get<5>(cell1_.instance_vector[iti])) {
          ins_.mod_name_ = "$lut";
          bool is_hex = false;
          int pos = std::get<3>(cell1_.instance_vector[iti]).find("h");
          if (pos != string::npos) {
            is_hex = true;
          }
          int width;

          if (string_compare(std::get<4>(cell1_.instance_vector[iti]), "")) {
            width = input_port_size;
          } else {

            width = stoi(std::get<4>(cell1_.instance_vector[iti]));
          }

          get_truth_table(std::get<3>(cell1_.instance_vector[iti]), width,
                          is_hex, ins_.truthTable_);

          sn.blocks.push_back(ins_);
        } else {
          sn.blocks.push_back(ins_);
        }
      }
    }
  }

  // The remaining nets are connected with the ports or ground or vcc so adding
  for (auto it = net_reduced_map.begin(); it != net_reduced_map.end(); it++) {

    for (long unsigned int i = 0; i < it->second.size(); i++) {

      // No need to connect the string port name with the string name and name
      // might be same but it should be of different instances
      if (((it->first != std::get<0>(it->second[i])) &&
           (std::get<0>(it->second[i]) != std::get<2>(it->second[i]))) &&
          (string_compare(std::get<1>(it->second[i]), ""))) {
        inst ins_;
        ins_.mod_name_ = "$lut";
        //
        ins_.conns_.push_back(std::make_pair(it->first, it->first));
        ins_.conns_.push_back(std::make_pair(std::get<0>(it->second[i]),
                                             std::get<0>(it->second[i])));
        get_truth_table("2", 1, false, ins_.truthTable_);
        sn.blocks.push_back(ins_);
      }
    }
  }

  sn.b_print(ss);
  fputs(ss.str().c_str(), edif_bl);
  snode_free(node);
}
