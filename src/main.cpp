//
// Created by kadius on 10.09.23.
//

#include <iostream>
#include <crow/app.h>
#include <crow/routing.h>
#include <crow/json.h>
#include <crow.h>
#include <pqxx/pqxx>
#include <cstdlib>
#include <signal.h>
#include <sstream>
#include <string>
#include <regex>
#include <cctype>
#include <filesystem>


using std::string;


auto main() -> int {
    std::cout << "[*] PROGRAM STATUS: RUNNING......" << std::endl;

    const string host = "postgres";
    const string port = "5432";
    const string db = "db";
    const string user = "user";
    const string password = "password";

    const auto url = "postgres://" + user + ":" + password + "@" + host + ":" +
                     port + "/" + db;

    pqxx::connection connection{url};

    crow::SimpleApp app;

    CROW_ROUTE(app, "/uploadfile")
            .methods(crow::HTTPMethod::Post)([](const crow::request& req) {

                std::cout << "[*] File upload path called" << std::endl;

                crow::multipart::message file_message(req);
                for (auto [part_name, part_value]: file_message.part_map) {
                    std::cout << "Part: " << part_name;
                    if ("InputFile" == part_name) {
                        // Extract the file name
                        auto headers_it = part_value.headers.find("Content-Disposition");
                        if (headers_it == part_value.headers.end()) {
                            std::cout << "No Content-Disposition found";
                            return crow::response(400);
                        }
                        auto params_it = headers_it->second.params.find("filename");
                        if (params_it == headers_it->second.params.end()) {
                            std::cout << "Part with name \"InputFile\" should have a file";
                            return crow::response(400);
                        }
                        const std::string outfile_name = params_it->second;

                        for (auto [part_header_name, part_header_val]: part_value.headers) {
                            std::cout << "Header: " << part_header_name << '=' << part_header_val.value;
                            for (auto [param_key, param_val]: part_header_val.params) {
                                std::cout << " Param: " << param_key << ',' << param_val;
                            }
                        }

                        // Create a new file with the extracted file name and write file contents to
                        std::string path = "/rest-api/csvdb/";
                        path += outfile_name;
                        std::cout << path << std::endl;
                        std::ofstream fout(path);
                        if (!fout) {
                            std::cout << " Write to file failed\n";
                            continue;
                        }
                        fout << part_value.body;
                        fout.close();
                        std::cout << " Contents written to " << outfile_name << '\n';
                    } else {
                        std::cout << " Value: " << part_value.body << '\n';
                    }
                }
                return crow::response(200, crow::json::wvalue{{"response","File uploaded"}});
            });

    CROW_ROUTE(app, "/query")
            .methods(crow::HTTPMethod::GET)([&](const crow::request& req){

                auto body = crow::json::load(req.body);
                if(!body)
                    return crow::response(400, "Invalid body");

                std::string filename;
                std::string operation;
                std::string select_col_names;

                try {
                    filename = body["filename"].s();
                    operation = body["operation"].s();
                    select_col_names = body["select_col_names"].s();
                } catch (const std::runtime_error& err) {
                    return crow::response(400, "Invalid body");
                }

                std::string path = "/rest-api/csvdb/";
                path += filename;
                std::ifstream fin;
                fin.open(path, std::ios::in);

                if(!fin) return crow::response(400, crow::json::wvalue{{"response","No such file found"}});

//                //parse
                std::string header;
                std::getline(fin, header);

                std::stringstream ss;
                ss << header;
                std::string col_name;
                std::vector<string> header_row;
                while(std::getline(ss, col_name, ','))
                    header_row.emplace_back(col_name);

                //remove " and ' '
                for(auto& s: header_row) {
                    s.erase(std::remove(s.begin(), s.end(), ' '), s.end());
                    if(s.front() == '"' && s.back() == '"')
                        s = s.substr(1, s.size()-2);
                }

                auto header_length = header_row.size();
                std::vector<string> types(header_length, "N");//inferred types of cols

                std::string current_raw;
                while(std::getline(fin, current_raw)) {

                    std::vector<string> row;

                    bool is_quoted = false;
                    int startIndex = 0;
                    int endIndex = 0;

                    auto trim = [](string text) -> string {
                        if (text.empty()) return text;
                        std::regex r("ab");
                        return text[0] == '"' && text.length() > 2 ?
                               std::regex_replace(text.substr(1, text.length() - 2), r, "c") :
                               std::regex_replace(text, r, "c");
                    };

                    //convert the row to the correct form
                    for (auto c: current_raw) {
                        if (c == '"') is_quoted = !is_quoted;

                        if (c == ',' && !is_quoted) {
                            string str = current_raw.substr(startIndex, endIndex - startIndex);
                            row.push_back(trim(str));
                            startIndex = endIndex + 1;
                        }
                        ++endIndex;
                    }
                    string str = current_raw.substr(startIndex, endIndex - startIndex);
                    row.push_back(trim(str));
                    //==================================
                    //infer types
                    for (size_t i{}; i < header_length; ++i) {
                        if (row[i].empty()) {
                            continue;
                        }

                        if(types[i] != "N") {
                            continue;
                        }

                        bool met_slash = false;
                        bool met_letter = false;
                        bool met_number = false;

                        for (auto c: row[i]) {
                            if (std::isalpha(c)) met_letter = true;
                            else if (c == '/') met_slash = true;
                            else met_number = true;
                        }

                        if (met_letter) types[i] = "VARCHAR(255)";
                        else if (met_slash && met_number) types[i] = "DATA";
                        else if (met_number) types[i] = "NUMERIC";
                    }
                }

                fin.close();
                std::unordered_map<string, string> headerToType;
                for(int i{}; i < header_row.size(); ++i)
                    headerToType[header_row[i]] = types[i];

                //parse

//                return crow::response(200, crow::json::wvalue{{"data", data}});
                std::cout << "[*] LOG: BEFORE TXN" << std::endl;
                std::string query = "CREATE TEMP TABLE IF NOT EXISTS TTable (";
                std::stringstream colnames_ss;
                select_col_names.erase(std::remove(select_col_names.begin(),
                                                   select_col_names.end(),
                                                   ' '), select_col_names.end());
                colnames_ss << select_col_names;

                std::vector<string> sel_col_names_vec;
                string tmpstr;
                while(std::getline(colnames_ss, tmpstr, ','))
                    sel_col_names_vec.emplace_back(tmpstr);

                for(int i{}; i < header_row.size(); ++i) {
                    query += header_row[i];
                    query += " ";
                    query += headerToType[header_row[i]];
                    query += ",";
                }
                query.back() = ')';
                std::cout << query << std::endl;

                auto txn = pqxx::work{connection};
                txn.exec(query);

                string header_trimmed = header;
                header_trimmed.erase(std::remove(header_trimmed.begin(), header_trimmed.end(), '"'), header_trimmed.end());
                std::string query2 = "COPY TTable(";
                query2 += header_trimmed;
                query2 += ") FROM '";
                query2 += path;
                query2 += "' DELIMITER ',' CSV HEADER";

                txn.exec(query2);

//                std::string q2 = "Month='FEB'";
                std::string query3 = "SELECT " + select_col_names + " FROM TTABLE";
//                query3 += " WHERE ";
//                query3 += q2;

                auto commit_txn = [](const std::string& query, pqxx::work& txn) {
                    const auto rows = txn.exec(query);
                    txn.exec("DROP TABLE TTable");

                    txn.commit();
                    std::vector<crow::json::wvalue> data;
                    size_t i{1};
                    for(const auto& row: rows) {
                        string str;
                        for(const auto& field: row) {
                            str += field.as<string>();
                            str += ",";
                        }
                        str.pop_back();
                        data.push_back(crow::json::wvalue{{std::to_string(i++),str}});
                    }

                    return crow::response(200, crow::json::wvalue{{"body",data}});
                };

                if(operation == "select")
                    return commit_txn(query3, txn);

                if(operation == "select where") {
                    query3 += " WHERE ";
                    //TODO:: solve problems with escaping ' character in conditions with VARCHAR
                    std::string q = body["conditions"].s();

                    query3 += q;

                    return commit_txn(query3, txn);
                }

                if(operation == "select order") {
                    query3 += " ORDER BY ";
                    std::string q = body["order"].s();
                    query3 += q;

                    return commit_txn(query3, txn);
                }

                if(operation == "select where order") {
                    query3 += " WHERE ";
                    std::string q = body["conditions"].s();
                    query3 += q;

                    query3 += " ORDER BY ";
                    q = body["order"].s();
                    query3 += q;

                    return commit_txn(query3, txn);
                }

                return crow::response(400, crow::json::wvalue{{"Error","Bad operation"}});
            });

    CROW_ROUTE(app, "/csv")
            .methods(crow::HTTPMethod::GET, crow::HTTPMethod::DELETE)([](const crow::request& req){

                switch(req.method) {
                    case crow::HTTPMethod::DELETE: {

                        auto body = crow::json::load(req.body);
                        if(!body)
                            return crow::response(400, "Invalid body");

                        std::string filename;
                        try {
                            filename = body["filename"].s();
                        } catch(std::runtime_error& e) {
                            return crow::response(400, "Invalid body");
                        }

                        std::string path = "/rest-api/csvdb/";
                        path += filename;

                        if(std::filesystem::exists(path.c_str())) {
                            std::filesystem::remove(path.c_str());
                            std::cout << "deleted" << std::endl;
                            return crow::response(200, crow::json::wvalue{{"response","Success"}});
                        } else {
                            return crow::response(400, crow::json::wvalue{{"response","File not found"}});
                        }
                    }
                    case crow::HTTPMethod::GET: {
                        using directory_iterator = std::filesystem::recursive_directory_iterator;

                        std::vector<crow::json::wvalue> data;

                        std::string path = "/rest-api/csvdb/";
                        for (const auto& entry : directory_iterator(path)) {
                            std::ifstream fin(entry.path());

                            std::string header;
                            std::getline(fin, header);

                            data.push_back(crow::json::wvalue{{entry.path().filename(), header}});
                        }
                        return crow::response(200, crow::json::wvalue{{"response",data}});
                    }
                }
            });

    app.port(18481).run();

    return 0;
}