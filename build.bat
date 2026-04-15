@echo off
echo Building DSA Project Server...
g++ -std=c++17 -O2 -o server.exe graph.cpp dijkstra.cpp astar.cpp blockchain.cpp traffic_sim.cpp satellite_world.cpp server.cpp -lws2_32
if %ERRORLEVEL% == 0 (
    echo Build successful! Run with: .\server.exe
) else (
    echo Build failed!
)
