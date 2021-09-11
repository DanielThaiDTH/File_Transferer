add_library (Server connector.cpp servers.cpp connector.h servers.h)
add_library (Client connector.cpp clients.cpp connector.h clients.h)
target_compile_features (Server PUBLIC cxx_std_17)
target_compile_features (Client PUBLIC cxx_std_17)