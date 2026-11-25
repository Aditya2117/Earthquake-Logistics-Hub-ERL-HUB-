#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <queue>
#include <memory>
#include <limits>
#include <cmath>
#include <filesystem>

// ERL-HUB: Post-Earthquake Multi-Agency Emergency Relief Logistics Hub
// Standard: C++17

namespace ReliefOps {
namespace Inventory {

class ReliefCargo {
    std::string name;
    std::string category;
    int quantity;
    double unitWeightKg;
    int priority;
public:
    ReliefCargo(std::string n = "Unknown", std::string c = "General", int q = 0,
                double w = 0.0, int p = 3)
        : name(std::move(n)), category(std::move(c)), quantity(q), unitWeightKg(w), priority(p) {
        if (quantity < 0 || unitWeightKg < 0) throw std::invalid_argument("Cargo quantity/weight cannot be negative");
    }
    const std::string& getName() const { return name; }
    const std::string& getCategory() const { return category; }
    int getQuantity() const { return quantity; }
    double getUnitWeightKg() const { return unitWeightKg; }
    int getPriority() const { return priority; }
    double totalWeightKg() const { return quantity * unitWeightKg; }
    void add(int q) { if (q < 0) throw std::invalid_argument("Cannot restock a negative quantity"); quantity += q; }
    void deduct(int q) { if (q < 0 || q > quantity) throw std::out_of_range("Insufficient cargo stock"); quantity -= q; }
    bool operator==(const ReliefCargo& rhs) const { return name == rhs.name && category == rhs.category; }
    friend std::ostream& operator<<(std::ostream& os, const ReliefCargo& c) {
        return os << std::left << std::setw(22) << c.name << std::setw(16) << c.category
                  << std::right << std::setw(8) << c.quantity << std::setw(12) << c.totalWeightKg();
    }
    friend std::istream& operator>>(std::istream& is, ReliefCargo& c) {
        std::string n, cat; int q, p; double w;
        if (is >> std::quoted(n) >> std::quoted(cat) >> q >> w >> p) {
            if (q < 0 || w < 0) { is.setstate(std::ios::failbit); return is; }
            c = ReliefCargo(n, cat, q, w, p);
        }
        return is;
    }
    std::string serialize() const {
        std::ostringstream out; out << std::quoted(name) << ' ' << std::quoted(category) << ' '
            << quantity << ' ' << unitWeightKg << ' ' << priority; return out.str();
    }
};

class DistributionDepot {
    std::string depotName;
    std::unordered_map<std::string, ReliefCargo> stock;
public:
    explicit DistributionDepot(std::string name) : depotName(std::move(name)) {}
    const std::string& getName() const { return depotName; }
    void restock(const ReliefCargo& cargo) {
        auto it = stock.find(cargo.getName());
        if (it == stock.end()) stock.emplace(cargo.getName(), cargo);
        else it->second.add(cargo.getQuantity());
    }
    ReliefCargo& getStock(const std::string& item) {
        auto it = stock.find(item);
        if (it == stock.end()) throw std::out_of_range("Cargo not found in depot: " + item);
        return it->second; // return by reference
    }
    DistributionDepot& operator+=(const ReliefCargo& cargo) { restock(cargo); return *this; } // depot restocking
    DistributionDepot& operator-=(const ReliefCargo& cargo) {
        getStock(cargo.getName()).deduct(cargo.getQuantity()); return *this; // departure deduction
    }
    bool operator==(const DistributionDepot& rhs) const { return depotName == rhs.depotName; }
    const std::unordered_map<std::string, ReliefCargo>& getAllStock() const { return stock; }
    void printStock() const {
        std::cout << "\nDEPOT: " << depotName << "\n" << std::left << std::setw(22) << "Cargo"
                  << std::setw(16) << "Category" << std::right << std::setw(8) << "Units" << std::setw(12) << "Kg" << '\n';
        for (const auto& entry : stock) std::cout << entry.second << '\n';
    }
};

} // Inventory

namespace Logistics {

using ReliefOps::Inventory::ReliefCargo;

inline double calculateDistanceKm(double x1, double y1, double x2, double y2) {
    const double dx = x2 - x1, dy = y2 - y1;
    return std::sqrt(dx * dx + dy * dy); // illustrative grid distance
}
inline double calculatePayloadTonnes(double kg) { return kg / 1000.0; }

class CargoOverweightException : public std::exception {
    std::string message;
public:
    explicit CargoOverweightException(const std::string& vehicle) : message("Cargo exceeds payload capacity of " + vehicle) {}
    const char* what() const noexcept override { return message.c_str(); }
};
class FuelDepletionException : public std::exception {
public: const char* what() const noexcept override { return "Vehicle fuel/battery reserve is insufficient for this mission"; }
};
class DamagedCorridorException : public std::exception {
    std::string message;
public:
    explicit DamagedCorridorException(const std::string& route) : message("Route corridor is marked damaged: " + route) {}
    const char* what() const noexcept override { return message.c_str(); }
};

struct GPSBeacon {
    double x, y;
    GPSBeacon(double px = 0, double py = 0) : x(px), y(py) {}
};
struct EmergencyLifeSupportUnit {
    bool operational;
    explicit EmergencyLifeSupportUnit(bool ok = true) : operational(ok) {}
};

class ReliefVehicle {
protected:
    std::string id;
    double payloadCapacityKg;
    double fuelPercent;
    std::string currentLocation;
    GPSBeacon gps;                         // composition
    EmergencyLifeSupportUnit lifeSupport; // composition
    std::vector<ReliefCargo> manifest;     // dynamically managed manifest
    static int operationalVehicles;
    static double totalDispatchedTonnes;
public:
    ReliefVehicle(std::string code, double capacity, double fuel, std::string location)
        : id(std::move(code)), payloadCapacityKg(capacity), fuelPercent(fuel),
          currentLocation(std::move(location)), gps(), lifeSupport(true) { ++operationalVehicles; }
    ReliefVehicle(const ReliefVehicle& other)
        : id(other.id + "-COPY"), payloadCapacityKg(other.payloadCapacityKg), fuelPercent(other.fuelPercent),
          currentLocation(other.currentLocation), gps(other.gps), lifeSupport(other.lifeSupport), manifest(other.manifest) {
        ++operationalVehicles;
    }
    virtual ~ReliefVehicle() { --operationalVehicles; }
    virtual std::string type() const = 0;
    virtual double speedKmph() const = 0;
    virtual bool canUseDamagedRoads() const { return false; }
    virtual void evaluateRoute(const std::string& route, bool damaged, double distanceKm) const {
        if (damaged && !canUseDamagedRoads()) throw DamagedCorridorException(route);
        if (fuelPercent < distanceKm / 10.0) throw FuelDepletionException();
        std::cout << type() << " " << id << " route " << route << " approved; ETA "
                  << std::fixed << std::setprecision(1) << (distanceKm / speedKmph()) * 60 << " minutes\n";
    }
    void loadCargo(const ReliefCargo& cargo) {
        double newWeight = cargo.totalWeightKg();
        for (const auto& c : manifest) newWeight += c.totalWeightKg();
        if (newWeight > payloadCapacityKg) throw CargoOverweightException(id);
        manifest.push_back(cargo);
    }
    double manifestWeightKg() const {
        double total = 0; for (const auto& c : manifest) total += c.totalWeightKg(); return total;
    }
    const std::string& getId() const { return id; }
    double getCapacity() const { return payloadCapacityKg; }
    static int getOperationalVehicles() { return operationalVehicles; }
    static double getTotalDispatchedTonnes() { return totalDispatchedTonnes; }
    static void recordDispatch(double kg) { totalDispatchedTonnes += calculatePayloadTonnes(kg); }
    void printManifest() const {
        std::cout << "Manifest " << id << " (" << type() << ") - " << manifest.size() << " item(s), "
                  << manifestWeightKg() << " kg\n";
        for (const auto& c : manifest) std::cout << "  " << c.getName() << ": " << c.getQuantity() << " units\n";
    }
    std::string manifestAudit() const {
        std::ostringstream out;
        for (const auto& c : manifest) out << c.getName() << ':' << c.getQuantity() << ';';
        return out.str();
    }
    friend class IncidentCommander;
};
int ReliefVehicle::operationalVehicles = 0;
double ReliefVehicle::totalDispatchedTonnes = 0.0;

// Virtual base resolves the diamond for a hybrid amphibious medical vehicle.
class HybridPlatform : virtual public ReliefVehicle {
public:
    HybridPlatform(std::string id, double cap, double fuel, std::string loc)
        : ReliefVehicle(std::move(id), cap, fuel, std::move(loc)) {}
};
class GroundConvoy : virtual public ReliefVehicle {
public:
    GroundConvoy(std::string id, double cap, double fuel, std::string loc)
        : ReliefVehicle(std::move(id), cap, fuel, std::move(loc)) {}
    std::string type() const override { return "Ground convoy"; }
    double speedKmph() const override { return 45.0; }
};
class AerialTransport : virtual public ReliefVehicle {
public:
    AerialTransport(std::string id, double cap, double fuel, std::string loc)
        : ReliefVehicle(std::move(id), cap, fuel, std::move(loc)) {}
    std::string type() const override { return "Aerial transport"; }
    double speedKmph() const override { return 150.0; }
};
class AmphibiousCraft : virtual public ReliefVehicle {
public:
    AmphibiousCraft(std::string id, double cap, double fuel, std::string loc)
        : ReliefVehicle(std::move(id), cap, fuel, std::move(loc)) {}
    std::string type() const override { return "Amphibious craft"; }
    double speedKmph() const override { return 30.0; }
    bool canUseDamagedRoads() const override { return true; }
};
class AmphibiousMedicalUnit : public GroundConvoy, public AmphibiousCraft, public HybridPlatform {
public:
    AmphibiousMedicalUnit(std::string id, double cap, double fuel, std::string loc)
        : ReliefVehicle(id, cap, fuel, loc), GroundConvoy(id, cap, fuel, loc),
          AmphibiousCraft(id, cap, fuel, loc), HybridPlatform(id, cap, fuel, loc) {}
    std::string type() const override { return "Amphibious medical hybrid"; }
    double speedKmph() const override { return 35.0; }
    bool canUseDamagedRoads() const override { return true; }
};
class SearchRescueDrone : public AerialTransport {
public:
    SearchRescueDrone(std::string id, double cap, double fuel, std::string loc)
        : ReliefVehicle(id, cap, fuel, loc), AerialTransport(id, cap, fuel, loc) {}
    std::string type() const override { return "Search-and-rescue drone"; }
    double speedKmph() const override { return 65.0; }
};

class IncidentCommander {
public:
    void emergencyOverride(ReliefVehicle& vehicle, double reserve) const {
        vehicle.fuelPercent = std::max(vehicle.fuelPercent, reserve); // friend access
        std::cout << "[COMMAND OVERRIDE] " << vehicle.id << " reserve set to " << vehicle.fuelPercent << "%\n";
    }
};

// Deep-copy dynamic manifest container (demonstrates dynamic constructor/copy/destructor).
class DynamicManifest {
    ReliefCargo* items;
    std::size_t count;
public:
    explicit DynamicManifest(std::size_t n = 0) : items(n ? new ReliefCargo[n] : nullptr), count(n) {}
    DynamicManifest(const DynamicManifest& other) : items(other.count ? new ReliefCargo[other.count] : nullptr), count(other.count) {
        for (std::size_t i = 0; i < count; ++i) items[i] = other.items[i];
    }
    DynamicManifest& operator=(const DynamicManifest& other) {
        if (this != &other) { ReliefCargo* copy = other.count ? new ReliefCargo[other.count] : nullptr;
            for (std::size_t i = 0; i < other.count; ++i) copy[i] = other.items[i];
            delete[] items; items = copy; count = other.count; }
        return *this;
    }
    ~DynamicManifest() { delete[] items; }
    ReliefCargo& operator[](std::size_t i) { if (i >= count) throw std::out_of_range("Manifest index"); return items[i]; }
    std::size_t size() const { return count; }
};

// Generic priority queue: lower numeric priority value is dispatched first.
template <typename T>
class PriorityDispatchQueue {
    struct Compare { bool operator()(const T& a, const T& b) const { return a.priority > b.priority; } };
    std::priority_queue<T, std::vector<T>, Compare> queue;
public:
    void push(const T& value) { queue.push(value); }
    T pop() { if (queue.empty()) throw std::out_of_range("Dispatch queue is empty"); T v = queue.top(); queue.pop(); return v; }
    bool empty() const { return queue.empty(); }
    std::size_t size() const { return queue.size(); }
};

struct Mission {
    std::string name;
    std::string destination;
    int priority;
    double distanceKm;
    std::string cargoName;
    int quantity;
    bool damagedRoute;
    std::string routeName;
};
struct UrgencyFunctor {
    bool operator()(const Mission& a, const Mission& b) const { return a.priority < b.priority; }
};
std::ostream& tableHeader(std::ostream& os) {
    return os << std::left << std::setw(18) << "MISSION" << std::setw(18) << "DESTINATION"
              << std::setw(10) << "PRIORITY" << std::setw(12) << "DISTANCE";
}
std::ostream& tableRow(std::ostream& os) { return os << "\n"; }

// Overloaded allocation: volume/payload based and emergency triage based.
void allocateSupply(Inventory::DistributionDepot& depot, ReliefVehicle& vehicle,
                    const std::string& item, int quantity) {
    auto& stock = depot.getStock(item); // reference update
    ReliefCargo shipment(item, stock.getCategory(), quantity, stock.getUnitWeightKg(), stock.getPriority());
    vehicle.loadCargo(shipment);
    stock.deduct(quantity);
}
void allocateSupply(Inventory::DistributionDepot& depot, ReliefVehicle& vehicle,
                    const std::string& item, int quantity, bool emergencyTriage) {
    auto& stock = depot.getStock(item);
    if (emergencyTriage && stock.getPriority() > 1)
        std::cout << "Triage override: allocating " << item << " for emergency medical mission.\n";
    allocateSupply(depot, vehicle, item, quantity);
}

void saveTransitLog(const Mission& m, const ReliefVehicle& v, double kg) {
    // Append-only binary records; rollback to previous end if write fails.
    std::fstream file("transit_log.bin", std::ios::in | std::ios::out | std::ios::binary);
    if (!file) { std::ofstream create("transit_log.bin", std::ios::binary); create.close();
        file.open("transit_log.bin", std::ios::in | std::ios::out | std::ios::binary); }
    file.seekp(0, std::ios::end);
    const std::streampos oldEnd = file.tellp();
    std::ostringstream record;
    record << m.name << '|' << m.destination << '|' << v.getId() << '|' << kg << '|' << m.priority << '|' << v.manifestAudit() << '\n';
    const std::string data = record.str();
    file.write(data.data(), static_cast<std::streamsize>(data.size()));
    if (!file) {
        file.clear(); file.close();
        std::filesystem::resize_file("transit_log.bin", static_cast<std::uintmax_t>(oldEnd));
        throw std::runtime_error("Transit log write failed; previous file length restored");
    }
    file.flush();
    // Read-back verification through seekg.
    file.seekg(oldEnd);
    std::string verify(data.size(), '\0');
    file.read(&verify[0], static_cast<std::streamsize>(verify.size()));
    if (verify != data) {
        file.close();
        std::filesystem::resize_file("transit_log.bin", static_cast<std::uintmax_t>(oldEnd));
        throw std::runtime_error("Transit log verification failed; previous file length restored");
    }
}

void restoreTransitLog() {
    std::ifstream file("transit_log.bin", std::ios::binary);
    if (!file) { std::cout << "No previous transit log found.\n"; return; }
    std::cout << "\nRESTORED TRANSIT / MANIFEST AUDIT RECORDS\n";
    std::string record;
    while (std::getline(file, record)) if (!record.empty()) std::cout << record << '\n';
}

} // Logistics
} // ReliefOps

int main() {
    using namespace ReliefOps;
    using namespace ReliefOps::Inventory;
    using namespace ReliefOps::Logistics;

    std::cout << "==============================================================\n"
              << " POST-EARTHQUAKE EMERGENCY RELIEF LOGISTICS HUB (ERL-HUB)\n"
              << "==============================================================\n";

    DistributionDepot depot("Ghaziabad Regional Relief Depot");
    depot.restock(ReliefCargo("Ration Kits", "Food", 500, 2.0, 2));
    depot.restock(ReliefCargo("Trauma Medicine", "Medical", 120, 0.5, 1));
    depot.restock(ReliefCargo("Clean Water", "Water", 300, 5.0, 2));
    depot.restock(ReliefCargo("Thermal Blankets", "Shelter", 250, 1.0, 3));

    // A fleet stored as base pointers demonstrates runtime polymorphism and virtual destruction.
    std::vector<std::unique_ptr<ReliefVehicle>> fleet;
    fleet.emplace_back(new GroundConvoy("TRK-101", 2500, 90, "Depot"));
    fleet.emplace_back(new AerialTransport("MED-202", 800, 85, "Depot"));
    fleet.emplace_back(new SearchRescueDrone("DRN-303", 25, 95, "Depot"));
    fleet.emplace_back(new AmphibiousMedicalUnit("AMP-404", 1800, 80, "Depot"));

    IncidentCommander commander;
    commander.emergencyOverride(*fleet[1], 90);

    std::vector<Mission> missions = {
        {"Trauma response", "City Hospital", 1, 18.0, "Trauma Medicine", 40, false, "North corridor"},
        {"Water distribution", "Sector 8", 2, 12.0, "Clean Water", 30, false, "East corridor"},
        {"Shelter support", "Village Camp", 3, 24.0, "Thermal Blankets", 5, false, "Village access road"},
        {"Flooded settlement", "River Colony", 4, 16.0, "Clean Water", 100, true, "Damaged river road"}
    };
    std::sort(missions.begin(), missions.end(), UrgencyFunctor());
    std::for_each(missions.begin(), missions.end(), [](const Mission& m) {
        if (m.priority == 1) std::cout << "[URGENT MEDICAL] " << m.name << " prioritized.\n";
    });

    PriorityDispatchQueue<Mission> dispatchQueue;
    for (const auto& mission : missions) dispatchQueue.push(mission);

    std::cout << "\nPRIORITY DISPATCH SCHEDULE\n";
    tableHeader(std::cout) << "\n";
    for (const auto& m : missions)
        std::cout << std::left << std::setw(18) << m.name << std::setw(18) << m.destination
                  << std::setw(10) << m.priority << std::setw(12) << m.distanceKm << tableRow;

    std::size_t vehicleIndex = 0;
    while (!dispatchQueue.empty()) {
        Mission mission = dispatchQueue.pop();
        ReliefVehicle& vehicle = *fleet[vehicleIndex % fleet.size()];
        ++vehicleIndex;
        try {
            if (mission.cargoName == "Trauma Medicine")
                allocateSupply(depot, vehicle, mission.cargoName, mission.quantity, true);
            else
                allocateSupply(depot, vehicle, mission.cargoName, mission.quantity);
            vehicle.evaluateRoute(mission.routeName, mission.damagedRoute, mission.distanceKm);
            const double kg = vehicle.manifestWeightKg();
            ReliefVehicle::recordDispatch(kg);
            saveTransitLog(mission, vehicle, kg);
            vehicle.printManifest();
            std::cout << "Dispatch completed and saved to binary transit log.\n";
        } catch (const CargoOverweightException& e) {
            std::cerr << "[CARGO ERROR] " << e.what() << '\n';
        } catch (const FuelDepletionException& e) {
            std::cerr << "[FUEL ERROR] " << e.what() << '\n';
        } catch (const DamagedCorridorException& e) {
            std::cerr << "[ROUTE ERROR] " << e.what() << '\n';
            if (vehicle.canUseDamagedRoads()) std::cerr << "Vehicle is permitted to use alternate damaged-road access.\n";
        } catch (const std::exception& e) {
            std::cerr << "[SYSTEM ERROR] " << e.what() << '\n';
        }
    }

    // Demonstrate depot operator overloading: += restock, -= dispatch deduction.
    depot += ReliefCargo("Ration Kits", "Food", 10, 2.0, 2);
    depot -= ReliefCargo("Ration Kits", "Food", 2, 2.0, 2);
    DistributionDepot auditDepot("Temporary Audit Depot");
    std::cout << "Depot identity check (same depot): " << (depot == auditDepot ? "same" : "different") << '\n';

    DynamicManifest originalManifest(1);
    originalManifest[0] = ReliefCargo("Audit Sample", "Test", 1, 0.1, 3);
    DynamicManifest copiedManifest = originalManifest; // deep copy constructor
    std::cout << "Dynamic manifest deep-copy check: " << copiedManifest.size() << " item(s) copied\n";
    ReliefCargo restoredCargo;
    std::istringstream cargoInput("\"Audit Box\" \"Relief\" 3 1.5 2");
    if (cargoInput >> restoredCargo)
        std::cout << "Stream extraction test: restored " << restoredCargo.getName() << " (" << restoredCargo.getQuantity() << " units)\n";

    depot.printStock();
    std::cout << "\nNETWORK ANALYTICS\nOperational vehicles: " << ReliefVehicle::getOperationalVehicles()
              << "\nTotal dispatched: " << ReliefVehicle::getTotalDispatchedTonnes() << " tonnes\n";
    restoreTransitLog();
    std::cout << "\nTransit records are stored in transit_log.bin in the program directory.\n";
    std::cout << "ERL-HUB session finished.\n";
    return 0;
}
