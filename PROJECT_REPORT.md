# Post-Earthquake Multi-Agency Emergency Relief Logistics Hub (ERL-HUB)

## 1. Problem statement

After an earthquake, multiple response agencies must distribute essential supplies while roads, bridges, and other infrastructure may be damaged. ERL-HUB is a C++ console prototype that coordinates inventory, transport resources, and priority-based dispatches. It demonstrates object-oriented programming mechanisms from all four units of CIC-211.

## 2. Objectives

- Maintain relief inventory at a regional distribution depot.
- Represent different transport modes using a common vehicle interface.
- Allocate cargo while checking payload limits.
- Prioritize emergency medical and other relief missions.
- Evaluate route access, vehicle reserve, and travel-time estimates.
- Preserve dispatch records and display network-wide logistics statistics.

## 3. System modules

### Inventory module — `ReliefOps::Inventory`

**ReliefCargo** stores cargo name, category, quantity, unit weight, and priority. It validates input, calculates total weight, and provides stream insertion/extraction and identity comparison.

**DistributionDepot** stores stock in an `std::unordered_map`. Its `getStock()` method returns a reference so stock can be updated without copying. `+=` restocks cargo, `-=` deducts cargo for dispatch, and `==` compares depot identity.

### Logistics module — `ReliefOps::Logistics`

**ReliefVehicle** is an abstract base class. It owns a manifest, payload and fuel information, plus composed `GPSBeacon` and `EmergencyLifeSupportUnit` objects. Static members track active vehicles and total dispatched tonnes.

Derived transport classes provide their own type and speed. `GroundConvoy`, `AerialTransport`, and `AmphibiousCraft` form the transport branches. `AmphibiousMedicalUnit` combines multiple branches using virtual inheritance to avoid duplicate `ReliefVehicle` base subobjects. `SearchRescueDrone` models a lightweight aerial search vehicle.

**IncidentCommander** is a friend class of `ReliefVehicle` and can apply an emergency fuel-reserve override.

**DynamicManifest** demonstrates dynamic array allocation, a parameterized constructor, deep-copy constructor, copy assignment, and destructor.

**PriorityDispatchQueue<T>** is a generic priority queue template. `Mission` objects are ordered so lower numeric priority values are dispatched first.

## 4. OOPS syllabus coverage

### Unit I — Core foundations and call semantics

| Requirement | Implementation |
|---|---|
| Namespaces | `ReliefOps::Logistics` and `ReliefOps::Inventory` |
| Function overloading | Two `allocateSupply()` overloads: standard payload allocation and emergency-triage allocation |
| Call by reference | Allocation functions receive depot and vehicle by reference; `getStock()` returns a reference |
| Return by reference | `DistributionDepot::getStock()` returns `ReliefCargo&` |
| Inline utilities | `calculateDistanceKm()` and `calculatePayloadTonnes()` |
| Friend class | `IncidentCommander` accesses protected/private vehicle state for an emergency override |

### Unit II — Encapsulation, dynamic memory, and operator overloading

| Requirement | Implementation |
|---|---|
| Encapsulation / information hiding | Private cargo and depot data; public controlled operations |
| Static data and functions | Active vehicle count and total dispatched tonnes in `ReliefVehicle` |
| Dynamic allocation | `DynamicManifest` allocates a dynamic `ReliefCargo` array |
| Deep copy and cleanup | Copy constructor, copy assignment, and destructor in `DynamicManifest` |
| `+=` | Restock cargo at a depot |
| `-=` | Deduct cargo from depot stock on departure |
| `==` | Compare cargo identity and depot identity |
| `<<` and `>>` | Cargo display and extraction operators |

### Unit III — Inheritance, composition, and polymorphism

| Requirement | Implementation |
|---|---|
| Base and derived classes | Abstract `ReliefVehicle` and concrete vehicle classes |
| Virtual inheritance / diamond resolution | `GroundConvoy`, `AmphibiousCraft`, and `HybridPlatform` virtually inherit `ReliefVehicle`; `AmphibiousMedicalUnit` combines them |
| Composition | Vehicle contains independent `GPSBeacon` and `EmergencyLifeSupportUnit` objects |
| Runtime polymorphism | Fleet uses `std::unique_ptr<ReliefVehicle>` and invokes virtual route evaluation/type/speed functions |
| Virtual destructor | `ReliefVehicle` has a virtual destructor, ensuring correct derived destruction |

### Unit IV — Streams, exceptions, and STL

| Requirement | Implementation |
|---|---|
| Custom exceptions | `CargoOverweightException`, `FuelDepletionException`, `DamagedCorridorException` derive from `std::exception` |
| Binary file stream | `std::fstream` opens `transit_log.bin` in binary mode |
| File positioning | Uses `seekp()` for append position and `seekg()` for read-back verification |
| Transaction-style rollback | If writing or verification fails, `std::filesystem::resize_file()` restores the previous file length |
| STL containers | `std::vector`, `std::unordered_map`, and `std::priority_queue` |
| STL algorithms | `std::sort` and `std::for_each` |
| Custom functors | `UrgencyFunctor` sorts urgent missions; queue comparator prioritizes lower numeric values |
| Custom manipulators | `tableHeader` and `tableRow` format the dispatch table |
| Generic template | `PriorityDispatchQueue<T>` |

## 5. Dispatch workflow

1. Initialize the depot with relief supplies.
2. Construct a fleet containing ground, aerial, drone, and amphibious transport vehicles.
3. Create missions with destination, priority, distance, cargo requirement, and route condition.
4. Sort missions using the urgency functor and add them to the generic dispatch queue.
5. Select a vehicle and allocate the requested cargo from depot stock.
6. Evaluate payload, fuel, and corridor restrictions through virtual vehicle methods.
7. For a successful dispatch, update tonnage analytics and persist the transit audit record.
8. Print remaining depot inventory and network statistics.

## 6. Exception handling

- **CargoOverweightException:** raised when the loaded manifest exceeds vehicle payload capacity.
- **FuelDepletionException:** raised when the vehicle reserve is insufficient for the mission estimate.
- **DamagedCorridorException:** raised when a route is damaged and the vehicle is not permitted to use it.
- Other standard exceptions are caught as system errors so one failed mission does not terminate the entire dispatch loop.

## 7. Sample testing

The source was compiled and run with:

```bash
g++ -std=c++17 -Wall -Wextra -pedantic main.cpp -o erl_hub
./erl_hub
```

The demonstration run successfully:
- Sorted four sample missions by urgency.
- Dispatched trauma medicine, clean water, thermal blankets, and water for a flooded settlement.
- Invoked all four transport implementations, including the amphibious hybrid on a damaged corridor.
- Updated depot quantities and dispatched-tonnage statistics.
- Created and verified `transit_log.bin` records.
- Demonstrated depot operators, dynamic-manifest deep copying, and stream extraction.

The sample data is designed for classroom demonstration. Route distance and travel-time calculations are simplified and must not be treated as operational guidance.

## 8. Conclusion

ERL-HUB demonstrates how C++ object-oriented design can organize emergency logistics. Encapsulation protects inventory and vehicle state, inheritance and polymorphism support a mixed fleet, templates and STL support priority dispatch, and file streams preserve an audit trail. The prototype provides a base that could be extended with a graphical interface, real GPS data, authenticated agency access, and a database.
