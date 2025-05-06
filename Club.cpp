#include <vector>
#include <string>
#include <map>
#include <list>
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <fstream>
#include <iostream>

using namespace std;

struct Table {
    bool is_occupied = false;
    string client_name;
    int start_time = 0; // in minutes
    int total_time = 0;
    int revenue = 0;
};

struct Client {
    bool is_seated = false;
    int table_id = -1;
};

int time_to_min(const string &time_str) {
    int hours = stoi(time_str.substr(0, 2));
    int minutes = stoi(time_str.substr(3, 2));
    return hours * 60 + minutes;
}

string min_to_time(int min) {
    int hours = min / 60;
    int minutes = min % 60;
    stringstream ss;
    ss << setfill('0') << setw(2) << hours << ":" << setfill('0') << setw(2) << minutes;
    return ss.str();
}

vector<string> split(const string &s) {
    vector<string> tokens;
    stringstream ss(s);
    string token;
    while (ss >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

bool is_valid_client_name(const string &name) {
    for (char c : name) {
        if (!(isdigit(c) || islower(c) || c == '_' || c == '-')) {
            return false;
        }
    }
    return true;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <input_file>" << endl;
        return 1;
    }

    ifstream input_file(argv[1]);
    if (!input_file.is_open()) {
        cerr << "Error opening file" << endl;
        return 1;
    }

    vector<string> lines;
    string line;
    while (getline(input_file, line)) {
        if (line.empty()) continue;
        lines.push_back(line);
    }

    input_file.close();

    if (lines.size() < 3) {
        cout << lines[0] << endl;
        return 0;
    }

    int num_tables;
    try {
        num_tables = stoi(lines[0]);
        if (num_tables <= 0) throw invalid_argument("");
    } catch (...) {
        cout << lines[0] << endl;
        return 0;
    }

    vector<string> time_tokens = split(lines[1]);
    if (time_tokens.size() != 2) {
        cout << lines[1] << endl;
        return 0;
    }
    int start_time, end_time;
    try {
        start_time = time_to_min(time_tokens[0]);
        end_time = time_to_min(time_tokens[1]);
        if (start_time >= end_time) throw invalid_argument("");
    } catch (...) {
        cout << lines[1] << endl;
        return 0;
    }

    int cost_per_hour;
    try {
        cost_per_hour = stoi(lines[2]);
        if (cost_per_hour <= 0) throw invalid_argument("");
    } catch (...) {
        cout << lines[2] << endl;
        return 0;
    }

    vector<string> event_lines;
    for (int i = 3; i < lines.size(); ++i) {
        event_lines.push_back(lines[i]);
    }

    int prev_event_time = -1;
    vector<vector<string>> parsed_events;
    for (const string &event_line : event_lines) {
        vector<string> tokens = split(event_line);
        if (tokens.size() < 2) {
            cout << event_line << endl;
            return 0;
        }

        string time_str = tokens[0];
        string id_str = tokens[1];
        int id;
        try {
            id = stoi(id_str);
            if (id < 1 || id > 4) throw invalid_argument("");
        } catch (...) {
            cout << event_line << endl;
            return 0;
        }

        int event_time;
        try {
            event_time = time_to_min(time_str);
        } catch (...) {
            cout << event_line << endl;
            return 0;
        }

        if (event_time < prev_event_time) {
            cout << event_line << endl;
            return 0;
        }
        prev_event_time = event_time;

        if (id == 1) {
            if (tokens.size() != 3 || !is_valid_client_name(tokens[2])) {
                cout << event_line << endl;
                return 0;
            }
        } else if (id == 2) {
            if (tokens.size() != 4 || !is_valid_client_name(tokens[2])) {
                cout << event_line << endl;
                return 0;
            }
            int table_id;
            try {
                table_id = stoi(tokens[3]);
                if (table_id < 1 || table_id > num_tables) throw invalid_argument("");
            } catch (...) {
                cout << event_line << endl;
                return 0;
            }
        } else if (id == 3 || id == 4) {
            if (tokens.size() != 3 || !is_valid_client_name(tokens[2])) {
                cout << event_line << endl;
                return 0;
            }
        } else {
            cout << event_line << endl;
            return 0;
        }

        parsed_events.push_back(tokens);
    }

    vector<Table> tables(num_tables);
    map<string, Client> clients;
    list<string> queue;
    vector<string> output_lines;
    output_lines.push_back(min_to_time(start_time));

    for (int i = 0; i < event_lines.size(); ++i) {
        const string &event_line = event_lines[i];
        output_lines.push_back(event_line);

        const vector<string> &tokens = parsed_events[i];
        int event_time = time_to_min(tokens[0]);
        int id = stoi(tokens[1]);
        string client_name = tokens[2];

        vector<string> generated_events;

        if (id == 1) {
            if (event_time < start_time || event_time >= end_time) {
                generated_events.push_back(min_to_time(event_time) + " 13 NotOpenYet");
            } else if (clients.count(client_name)) {
                generated_events.push_back(min_to_time(event_time) + " 13 YouShallNotPass");
            } else {
                clients[client_name] = Client{false, -1};
            }
        } else if (id == 2) {
            if (!clients.count(client_name)) {
                generated_events.push_back(min_to_time(event_time) + " 13 ClientUnknown");
            } else {
                int table_id = stoi(tokens[3]) - 1;
                if (table_id < 0 || table_id >= num_tables) {
                    generated_events.push_back(min_to_time(event_time) + " 13 PlaceIsBusy");
                } else if (tables[table_id].is_occupied) {
                    generated_events.push_back(min_to_time(event_time) + " 13 PlaceIsBusy");
                } else {
                    Client &client = clients[client_name];
                    if (client.is_seated) {
                        int prev_table = client.table_id;
                        tables[prev_table].is_occupied = false;
                        int duration = event_time - tables[prev_table].start_time;
                        tables[prev_table].total_time += duration;
                        int hours = (duration + 59) / 60;
                        tables[prev_table].revenue += hours * cost_per_hour;
                    }
                    tables[table_id].is_occupied = true;
                    tables[table_id].client_name = client_name;
                    tables[table_id].start_time = event_time;
                    client.is_seated = true;
                    client.table_id = table_id;

                    auto it = find(queue.begin(), queue.end(), client_name);
                    if (it != queue.end()) {
                        queue.erase(it);
                    }
                }
            }
        } else if (id == 3) {
            if (!clients.count(client_name)) {
                generated_events.push_back(min_to_time(event_time) + " 13 ClientUnknown");
            } else {
                bool has_free_table = false;
                for (const auto &table : tables) {
                    if (!table.is_occupied) {
                        has_free_table = true;
                        break;
                    }
                }
                if (has_free_table) {
                    generated_events.push_back(min_to_time(event_time) + " 13 ICanWaitNoLonger!");
                } else if (queue.size() >= num_tables) {
                    generated_events.push_back(min_to_time(event_time) + " 11 " + client_name);
                    clients.erase(client_name);
                    auto it = find(queue.begin(), queue.end(), client_name);
                    if (it != queue.end()) {
                        queue.erase(it);
                    }
                } else {
                    queue.push_back(client_name);
                    clients[client_name].is_seated = false;
                    clients[client_name].table_id = -1;
                }
            }
        } else if (id == 4) {
            if (!clients.count(client_name)) {
                generated_events.push_back(min_to_time(event_time) + " 13 ClientUnknown");
            } else {
                Client &client = clients[client_name];
                if (client.is_seated) {
                    int table_id = client.table_id;
                    tables[table_id].is_occupied = false;
                    int duration = event_time - tables[table_id].start_time;
                    tables[table_id].total_time += duration;
                    int hours = (duration + 59) / 60;
                    tables[table_id].revenue += hours * cost_per_hour;
                }
                clients.erase(client_name);
                auto it = find(queue.begin(), queue.end(), client_name);
                if (it != queue.end()) {
                    queue.erase(it);
                }

                for (int table_id = 0; table_id < num_tables; ++table_id) {
                    if (!tables[table_id].is_occupied && !queue.empty()) {
                        string waiting_client = queue.front();
                        queue.pop_front();

                        tables[table_id].is_occupied = true;
                        tables[table_id].client_name = waiting_client;
                        tables[table_id].start_time = event_time;
                        clients[waiting_client].is_seated = true;
                        clients[waiting_client].table_id = table_id;

                        generated_events.push_back(min_to_time(event_time) + " 12 " + waiting_client + " " + to_string(table_id + 1));
                        break;
                    }
                }
            }
        }

        for (const auto &e : generated_events) {
            output_lines.push_back(e);
        }
    }

    vector<string> remaining_clients;
    for (const auto &pair : clients) {
        remaining_clients.push_back(pair.first);
    }
    sort(remaining_clients.begin(), remaining_clients.end());

    for (const string &client : remaining_clients) {
        Client &c = clients[client];
        if (c.is_seated) {
            int table_id = c.table_id;
            int duration = end_time - tables[table_id].start_time;
            tables[table_id].total_time += duration;
            int hours = (duration + 59) / 60;
            tables[table_id].revenue += hours * cost_per_hour;
            tables[table_id].is_occupied = false;
        }
        output_lines.push_back(min_to_time(end_time) + " 11 " + client);
    }
    output_lines.push_back(min_to_time(end_time));

    for (const string &line : output_lines) {
        cout << line << endl;
    }

    for (int i = 0; i < num_tables; ++i) {
        cout << i + 1 << " " << tables[i].revenue << " " << min_to_time(tables[i].total_time) << endl;
    }

    return 0;
}