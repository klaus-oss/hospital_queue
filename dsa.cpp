#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <map>
#include <cctype>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <limits>

#ifdef _WIN32
  #include <conio.h>
  #define CLEAR "cls"
#else
  #include <termios.h>
  #include <unistd.h>
  #define CLEAR "clear"
#endif

using namespace std;

// ---------- cross‑platform getch and clear ----------
void clear() { system(CLEAR); }
void press() { cout << "\nPress Enter..."; cin.ignore(numeric_limits<streamsize>::max(), '\n'); }

char getch() {
#ifdef _WIN32
    return _getch();
#else
    termios old, newt;
    tcgetattr(0, &old);
    newt = old;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(0, TCSANOW, &newt);
    char ch = getchar();
    tcsetattr(0, TCSANOW, &old);
    return ch;
#endif
}

// ---------- helpers ----------
string trim(string s) {
    s.erase(0, s.find_first_not_of(" \t\n\r"));
    s.erase(s.find_last_not_of(" \t\n\r") + 1);
    return s;
}
string toLower(string s) { for (char& c : s) c = tolower(c); return s; }
string nowDate() {
    time_t t = time(0); tm* now = localtime(&t);
    ostringstream oss;
    oss << (now->tm_year+1900) << "-" << setw(2) << setfill('0') << (now->tm_mon+1) << "-" << setw(2) << setfill('0') << now->tm_mday;
    return oss.str();
}

string getStr(string p) {
    string s;
    while (cout << p, getline(cin, s), s = trim(s), s.empty()) cout << "Cannot be empty.\n";
    return s;
}
string getName(string p) {
    string s;
    while (true) {
        s = getStr(p);
        bool ok = true;
        for (char c : s) if (!isalpha(c) && c != ' ' && c != '-') { ok = false; break; }
        if (ok) return s;
        cout << "Use letters, spaces or hyphens.\n";
    }
}
char getGender() {
    char g;
    while (cout << "Gender (M/F): ", g = toupper(getch()), cout << g << endl, g != 'M' && g != 'F')
        cout << "Invalid.\n";
    return g;
}
int getAge() {
    int a;
    while (cout << "Age (0-150): ", cin >> a, cin.fail() || a < 0 || a > 150)
        { cin.clear(); cin.ignore(1e6,'\n'); cout << "Invalid.\n"; }
    cin.ignore();
    return a;
}
int getInt(string p, int minv=1, int maxv=1e6) {
    int n;
    while (cout << p, cin >> n, cin.fail() || n < minv || n > maxv)
        { cin.clear(); cin.ignore(1e6,'\n'); cout << "Invalid.\n"; }
    cin.ignore();
    return n;
}
bool yesNo(string p) {
    string s;
    while (true) {
        cout << p << " (y/n): "; getline(cin, s); s = trim(s);
        if (s == "y" || s == "Y") return true;
        if (s == "n" || s == "N") return false;
        cout << "Please enter y or n.\n";
    }
}

// ---------- data structures ----------
struct Prescription {
    int id; string med, dose, instr, date, doctor;
    Prescription(string m, string d, string i, string doc) : med(m), dose(d), instr(i), doctor(doc) {
        static int nextId = 1;
        id = nextId++; date = nowDate();
    }
    Prescription(string m, string d, string i, string doc, string dt, int id_) : med(m), dose(d), instr(i), doctor(doc), date(dt), id(id_) {}
};

struct History {
    int id; string act, res, doc, date;
    History(string a, string r, string d) : act(a), res(r), doc(d) {
        static int nextId = 1;
        id = nextId++; date = nowDate();
    }
    History(string a, string r, string d, string dt, int id_) : act(a), res(r), doc(d), date(dt), id(id_) {}
};

struct Patient {
    int id, age;
    string f, m, l;
    char gender;
    vector<History> history;
    vector<Prescription> prescriptions;
    Patient(int i, string f_, string m_, string l_, int a, char g) : id(i), age(a), f(f_), m(m_), l(l_), gender(g) {}
};

vector<Patient> patients;                     // all patients
vector<pair<int, string>> activeQueue;        // (patientId, "emergency"/"Regular")

string dataFile = "medsys.txt", userFile = "users.txt";

// ---------- file I/O ----------
void save() {
    ofstream f(dataFile.c_str());
    f << patients.size() << '\n';
    for (auto& p : patients) {
        f << p.id << '|' << p.f << '|' << p.m << '|' << p.l << '|' << p.age << '|' << p.gender << '\n';
        f << p.history.size() << '\n';
        for (auto& h : p.history)
            f << h.id << '|' << h.act << '|' << h.res << '|' << h.doc << '|' << h.date << '\n';
        f << p.prescriptions.size() << '\n';
        for (auto& r : p.prescriptions)
            f << r.id << '|' << r.med << '|' << r.dose << '|' << r.instr << '|' << r.doctor << '|' << r.date << '\n';
    }
    f << activeQueue.size() << '\n';
    for (auto& q : activeQueue) f << q.first << '|' << q.second << '\n';
    f.close();
}

void load() {
    ifstream f(dataFile.c_str());
    if (!f) return;
    int n; f >> n; f.ignore();
    patients.clear();
    for (int i = 0; i < n; ++i) {
        string line; getline(f, line);
        size_t p1 = line.find('|'), p2 = line.find('|', p1+1), p3 = line.find('|', p2+1), p4 = line.find('|', p3+1);
        int id = stoi(line.substr(0, p1));
        string fname = line.substr(p1+1, p2-p1-1), mname = line.substr(p2+1, p3-p2-1), lname = line.substr(p3+1, p4-p3-1);
        int age = stoi(line.substr(p4+1));
        char g = line.back();
        patients.push_back(Patient(id, fname, mname, lname, age, g));
        Patient& p = patients.back();
        int hcnt; f >> hcnt; f.ignore();
        for (int j = 0; j < hcnt; ++j) {
            getline(f, line);
            size_t q1 = line.find('|'), q2 = line.find('|', q1+1), q3 = line.find('|', q2+1), q4 = line.find('|', q3+1);
            int hid = stoi(line.substr(0, q1));
            string a = line.substr(q1+1, q2-q1-1), r = line.substr(q2+1, q3-q2-1), d = line.substr(q3+1, q4-q3-1), dt = line.substr(q4+1);
            p.history.push_back(History(a, r, d, dt, hid));
        }
        int rcnt; f >> rcnt; f.ignore();
        for (int j = 0; j < rcnt; ++j) {
            getline(f, line);
            size_t q1 = line.find('|'), q2 = line.find('|', q1+1), q3 = line.find('|', q2+1), q4 = line.find('|', q3+1), q5 = line.find('|', q4+1);
            int rid = stoi(line.substr(0, q1));
            string med = line.substr(q1+1, q2-q1-1), dose = line.substr(q2+1, q3-q2-1), instr = line.substr(q3+1, q4-q3-1), doc = line.substr(q4+1, q5-q4-1), dt = line.substr(q5+1);
            p.prescriptions.push_back(Prescription(med, dose, instr, doc, dt, rid));
        }
    }
    int qcnt; f >> qcnt; f.ignore();
    activeQueue.clear();
    for (int i = 0; i < qcnt; ++i) {
        string line; getline(f, line);
        size_t pos = line.find('|');
        int pid = stoi(line.substr(0, pos));
        string prio = line.substr(pos+1);
        activeQueue.push_back({pid, prio});
    }
    f.close();
}

// ---------- queue helpers ----------
bool inActive(int id) {
    for (auto& q : activeQueue) if (q.first == id) return true;
    return false;
}
void addToQueue(int id, bool emergency) {
    if (inActive(id)) { cout << "Already in queue.\n"; return; }
    if (emergency) activeQueue.insert(activeQueue.begin(), {id, "emergency"});
    else activeQueue.push_back({id, "Regular"});
    cout << "Added to " << (emergency ? "EMERGENCY" : "REGULAR") << " queue.\n";
    save();
}
void remFromQueue(int id, bool silent = false) {
    for (auto it = activeQueue.begin(); it != activeQueue.end(); ++it)
        if (it->first == id) { activeQueue.erase(it); if (!silent) cout << "Removed from queue.\n"; save(); return; }
    if (!silent) cout << "Not in queue.\n";
}
// find patient by id
Patient* findPatient(int id) {
    for (auto& p : patients) if (p.id == id) return &p;
    return nullptr;
}
// duplicate check including age
bool isDuplicate(const Patient& newP, int excludeId = -1) {
    for (auto& p : patients)
        if (p.id != excludeId && p.f == newP.f && p.m == newP.m && p.l == newP.l && p.gender == newP.gender && p.age == newP.age)
            return true;
    return false;
}
// show patient
void showPatient(const Patient& p, bool showHist=false, bool showPresc=false) {
    cout << "\nID: " << p.id << "\nName: " << p.f << " " << p.m << " " << p.l << "\nAge: " << p.age << "\nGender: " << p.gender << "\n";
    if (showHist) {
        cout << "--- History ---\n";
        for (auto& h : p.history) cout << h.id << " | " << h.act << " | " << h.res << " | " << h.date << " | " << h.doc << "\n";
    }
    if (showPresc) {
        cout << "--- Prescriptions ---\n";
        for (auto& r : p.prescriptions) cout << r.id << " | " << r.med << " | " << r.dose << " | " << r.instr << " | " << r.date << " | Dr." << r.doctor << "\n";
    }
}

// ---------- menu ----------
int menu(string title, vector<string> opts, string zero="Back") {
    while (true) {
        cout << "\n========= " << title << " =========\n";
        for (size_t i=0; i<opts.size(); ++i) cout << " [" << i+1 << "] " << opts[i] << "\n";
        cout << " [0] " << zero << "\nChoice: ";
        string s; getline(cin, s); s = trim(s);
        if (s.empty()) continue;
        if (!isdigit(s[0])) { cout << "Number.\n"; continue; }
        int ch = stoi(s);
        if (ch>=0 && ch<=(int)opts.size()) { clear(); return ch; }
        cout << "Invalid.\n";
    }
}

// ---------- features ----------
void registerPatient() {
    string f = getName("First: "), m = getName("Middle: "), l = getName("Last: ");
    int a = getAge(); char g = getGender();
    Patient temp(0, f, m, l, a, g);
    if (isDuplicate(temp)) {
        cout << "⚠ Duplicate patient (same name, gender, age).\n";
        if (yesNo("Add existing patient to queue instead?")) {
            for (auto& p : patients)
                if (p.f == f && p.m == m && p.l == l && p.gender == g && p.age == a) {
                    int choice = menu("Add to queue?", {"Emergency","Regular"},"Skip");
                    if (choice) addToQueue(p.id, choice==1);
                    else cout << "Not added.\n";
                    break;
                }
        } else cout << "Registration cancelled.\n";
        press(); return;
    }
    int newId = 1;
    for (auto& p : patients) if (p.id >= newId) newId = p.id + 1;
    patients.push_back(Patient(newId, f, m, l, a, g));
    cout << "Registered.\n";
    save();
    int choice = menu("Add to queue?", {"Emergency","Regular"},"Skip");
    if (choice) addToQueue(newId, choice==1);
    press();
}

void serve() {
    if (activeQueue.empty()) { cout << "No active patients.\n"; press(); return; }
    int ch = menu("Serve", {"Emergency first","Regular first","By ID"});
    Patient* p = nullptr;
    if (ch == 1) {
        auto it = activeQueue.begin();
        while (it != activeQueue.end() && it->second != "emergency") ++it;
        if (it == activeQueue.end()) { cout << "No emergency.\n"; press(); return; }
        p = findPatient(it->first);
        if (p) activeQueue.erase(it);
    } else if (ch == 2) {
        auto it = activeQueue.begin();
        while (it != activeQueue.end() && it->second != "Regular") ++it;
        if (it == activeQueue.end()) { cout << "No regular.\n"; press(); return; }
        p = findPatient(it->first);
        if (p) activeQueue.erase(it);
    } else if (ch == 3) {
        int id = getInt("ID: ");
        p = findPatient(id);
        if (!p) return;
        if (!inActive(id)) { cout << "Not in queue.\n"; press(); return; }
        remFromQueue(id, true);
    }
    if (p) {
        string act = getStr("Action: "), res = getStr("Result: "), doc = getStr("Doctor: ");
        p->history.push_back(History(act, res, doc));
        if (yesNo("Add prescription?")) {
            string med = getStr("Medicine: "), dose = getStr("Dosage: "), instr = getStr("Instructions: ");
            p->prescriptions.push_back(Prescription(med, dose, instr, doc));
            cout << "Prescription added.\n";
        }
        cout << "Service recorded.\n";
        save();
    }
    press();
}

void showAll(bool prescOnly=false) {
    for (auto& p : patients) showPatient(p, !prescOnly, prescOnly);
    press();
}
void searchById() { int id=getInt("ID: "); if(auto p=findPatient(id)) showPatient(*p); else cout<<"Not found.\n"; press(); }
void showWithHistory() { int id=getInt("ID: "); if(auto p=findPatient(id)) showPatient(*p,true); press(); }
void showWithPrescriptions() { int id=getInt("ID: "); if(auto p=findPatient(id)) showPatient(*p,false,true); press(); }
void fullDetails() { int id=getInt("ID: "); if(auto p=findPatient(id)) showPatient(*p,true,true); press(); }

// *** MODIFIED: sortPatients stays in its own submenu ***
void sortPatients() {
    if (patients.empty()) { cout << "No patients.\n"; press(); return; }
    int choice;
    do {
        choice = menu("SORT PATIENTS", {"By ID (asc)", "By Name (A-Z)"});
        if (choice == 0) break;
        if (choice == 1)
            sort(patients.begin(), patients.end(), [](auto& a, auto& b) { return a.id < b.id; });
        else if (choice == 2)
            sort(patients.begin(), patients.end(), [](auto& a, auto& b) {
                return toLower(a.f + " " + a.m + " " + a.l) < toLower(b.f + " " + b.m + " " + b.l);
            });
        clear();
        for (auto& p : patients) showPatient(p);
        press();
        clear();
    } while (choice != 0);
}

void searchByName() {
    string q = toLower(getStr("Name part: "));
    for (auto& p : patients)
        if (toLower(p.f+" "+p.m+" "+p.l).find(q) != string::npos) showPatient(p);
    press();
}
void editPatient(Patient& p) {
    int ch;
    do {
        clear(); showPatient(p);
        ch = menu("EDIT", {"First","Middle","Last","Age","Gender"});
        if (ch==0) break;
        string old;
        switch(ch) {
            case 1: old=p.f; p.f=getName("New first: "); break;
            case 2: old=p.m; p.m=getName("New middle: "); break;
            case 3: old=p.l; p.l=getName("New last: "); break;
            case 4: { int oldAge=p.age; p.age=getAge(); if(isDuplicate(p,p.id)) { cout<<"Duplicate! Revert.\n"; p.age=oldAge; } } break;
            case 5: old=string(1,p.gender); p.gender=getGender(); if(isDuplicate(p,p.id)) { cout<<"Duplicate! Revert.\n"; p.gender=old[0]; } break;
        }
        if (ch<=3 && isDuplicate(p,p.id)) { cout<<"Duplicate! Revert.\n"; if(ch==1) p.f=old; else if(ch==2) p.m=old; else p.l=old; }
        else cout << "Changed.\n";
        save();
        press(); clear();
    } while(true);
}
void deletePatient() {
    int id=getInt("ID: ");
    auto it = find_if(patients.begin(), patients.end(), [id](auto& p){ return p.id==id; });
    if (it == patients.end()) { cout << "Not found.\n"; press(); return; }
    if (!yesNo("Really delete?")) return;
    remFromQueue(id, true);
    patients.erase(it);
    cout << "Deleted.\n";
    save(); press();
}
void manageHistory(Patient& p) {
    if (p.history.empty()) { cout << "No history.\n"; press(); return; }
    for (auto& h : p.history) cout << "[" << h.id << "] " << h.act << " | " << h.res << " | " << h.date << " | " << h.doc << "\n";
    int hid = getInt("ID to edit/delete (0 cancel): ", 0);
    if (hid==0) return;
    auto hit = find_if(p.history.begin(), p.history.end(), [hid](auto& h){ return h.id==hid; });
    if (hit == p.history.end()) { cout << "Invalid.\n"; press(); return; }
    int act = menu("Action", {"Edit","Delete"});
    if (act==2) p.history.erase(hit);
    else {
        hit->act = getStr("New action: ");
        hit->res = getStr("New result: ");
        hit->doc = getStr("New doctor: ");
        cout << "Updated.\n";
    }
    save(); press();
}
void showStats() {
    int total=patients.size(), actQ=activeQueue.size(), withHist=0, totalHist=0, withPresc=0, totalPresc=0;
    map<string,int> acts, reses, meds;
    for (auto& p : patients) {
        if (!p.history.empty()) withHist++;
        totalHist += p.history.size();
        for (auto& h : p.history) { acts[h.act]++; reses[h.res]++; }
        if (!p.prescriptions.empty()) withPresc++;
        totalPresc += p.prescriptions.size();
        for (auto& r : p.prescriptions) meds[r.med]++;
    }
    cout << "\nStatistics:\nTotal patients: " << total << "\nActive in queue: " << actQ
         << "\nWith history: " << withHist << "\nTotal records: " << totalHist
         << "\nWith prescriptions: " << withPresc << "\nTotal prescriptions: " << totalPresc << "\n";
    auto top3 = [](map<string,int>& m) {
        vector<pair<string,int>> v(m.begin(), m.end());
        sort(v.begin(), v.end(), [](auto& a, auto& b) { return a.second > b.second; });
        for (size_t i=0; i<v.size() && i<3; ++i) cout << "  " << v[i].first << ": " << v[i].second << "\n";
    };
    cout << "Top actions:\n"; top3(acts);
    cout << "Top results:\n"; top3(reses);
    cout << "Top medicines:\n"; top3(meds);
    press();
}
void showActive() {
    if (activeQueue.empty()) cout << "No active patients.\n";
    else for (auto& q : activeQueue) if (auto p = findPatient(q.first)) cout << "ID: " << p->id << " | " << p->f << " " << p->m << " " << p->l << " | " << q.second << "\n";
    press();
}

// ---------- authentication ----------
struct User { string name, pass, role; };
User currentUser;
void initUsers() {
    ifstream f(userFile.c_str());
    if (!f) {
        ofstream out(userFile.c_str());
        out << "reception|rec123|receptionist\ndoctor|doc123|doctor\npharmacist|phar123|pharmacist\n";
    }
}
bool login() {
    while (true) {
        clear();
        int rch = menu("Login", {"Receptionist","Doctor","Pharmacist"},"Exit");
        if (rch==0) return false;
        string role = (rch==1?"receptionist":rch==2?"doctor":"pharmacist");
        string user = getStr("Username: ");
        cout << "Password: ";
        string pass = "";
        char ch;
        while ((ch=getch()) != '\r' && ch != '\n') {
            if (ch == '\b' || ch == 127) { if (!pass.empty()) { pass.pop_back(); cout << "\b \b"; } }
            else if (ch >= 32 && ch <= 126) { pass.push_back(ch); cout << '*'; }
        }
        cout << endl;
        ifstream f(userFile.c_str());
        string line;
        bool ok = false;
        while (getline(f, line)) {
            line = trim(line);
            size_t p1 = line.find('|'), p2 = line.find('|', p1+1);
            if (p1 != string::npos && p2 != string::npos) {
                string u = line.substr(0,p1), p = line.substr(p1+1,p2-p1-1), r = line.substr(p2+1);
                if (u == user && p == pass && r == role) { ok = true; break; }
            }
        }
        if (ok) {
            currentUser = {user, pass, role};
            cout << "Welcome, " << user << " (" << role << ")!\n";
            press(); clear();
            return true;
        } else { cout << "Invalid.\n"; press(); }
    }
}

// ---------- role-based menus ----------
void patientRecords() {
    vector<string> opts = {"Show All","Search by ID"};
    if (currentUser.role == "pharmacist")
        opts.insert(opts.end(), {"Show Prescriptions","Full Details","Sort Patients","Search by Name"});
    else
        opts.insert(opts.end(), {"Show with History","Sort Patients","Search by Name"});
    while (true) {
        int ch = menu("Patient Records", opts);
        if (ch==0) break;
        if (ch==1) showAll(currentUser.role=="pharmacist");
        else if (ch==2) searchById();
        else if (ch==3) currentUser.role=="pharmacist" ? showWithPrescriptions() : showWithHistory();
        else if (ch==4) currentUser.role=="pharmacist" ? fullDetails() : sortPatients();
        else if (ch==5) currentUser.role=="pharmacist" ? sortPatients() : searchByName();
        else if (ch==6 && currentUser.role=="pharmacist") searchByName();
    }
}
void managePatient() {
    if (currentUser.role == "pharmacist") { cout << "Access denied.\n"; press(); return; }
    int id = getInt("Patient ID: ");
    Patient* p = findPatient(id);
    if (!p) return;
    clear(); showPatient(*p);
    vector<string> opts = {"Edit Info","Delete Patient","Remove from Queue"};
    if (currentUser.role == "doctor") opts.push_back("Edit History");
    while (true) {
        int ch = menu("MANAGE PATIENT", opts);
        if (ch==0) break;
        if (ch==1) editPatient(*p);
        else if (ch==2) deletePatient();
        else if (ch==3) remFromQueue(p->id);
        else if (ch==4) manageHistory(*p);
        if (ch!=2) { clear(); showPatient(*p); }
        else break;
    }
}
void dashboard() {
    vector<string> opts;
    if (currentUser.role == "receptionist")
        opts = {"New Patient","Active Queue","Patient Records","Manage Patient","Statistics"};
    else if (currentUser.role == "doctor")
        opts = {"Active Queue","Serve Next","Patient Records","Manage Patient","Statistics"};
    else
        opts = {"Patient Records","Statistics"};
    while (true) {
        int ch = menu("MEDSYS // " + currentUser.role, opts, "Logout");
        if (ch==0) break;
        if (currentUser.role == "receptionist") {
            if (ch==1) registerPatient();
            else if (ch==2) showActive();
            else if (ch==3) patientRecords();
            else if (ch==4) managePatient();
            else if (ch==5) showStats();
        } else if (currentUser.role == "doctor") {
            if (ch==1) showActive();
            else if (ch==2) serve();
            else if (ch==3) patientRecords();
            else if (ch==4) managePatient();
            else if (ch==5) showStats();
        } else {
            if (ch==1) patientRecords();
            else if (ch==2) showStats();
        }
    }
}

// ---------- main ----------
int main() {
    load();
    initUsers();
    while (login()) dashboard();
    save();
    return 0;
}