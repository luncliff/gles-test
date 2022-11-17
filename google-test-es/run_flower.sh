#!/bin/bash
./test_shadow -frames 1000 -app "mesh_pn2 mesh/KotyouRan.mesh2 1" -app "albedo_map mesh/KotyouT.raw 256 256" -app "rotation_axis 0 1 0" -screen "640 640 60" -fsaa 4
./test_shadow -frames 1000 -app "mesh_pn2 mesh/Cattleya.mesh2 1" -app "albedo_map mesh/CattleT.raw 256 256" -app "rotation_axis 0 1 0" -screen "640 640 60" -fsaa 4
./test_shadow -frames 1000 -app "mesh_pn2 mesh/HakuMokuren.mesh2 1" -app "albedo_map mesh/HakuMoT.raw 256 256" -app "rotation_axis 0 1 0" -screen "640 640 60" -fsaa 4
./test_shadow -frames 1000 -app "mesh_pn2 mesh/HimawariS.mesh2 1" -app "albedo_map mesh/HimawaT.raw 256 256" -app "rotation_axis 0 1 0" -screen "640 640 60" -fsaa 4
./test_shadow -frames 1000 -app "mesh_pn2 mesh/Poinsettia.mesh2 1" -app "albedo_map mesh/PoinseT.raw 256 256" -app "rotation_axis 0 1 0" -screen "640 640 60" -fsaa 4
./test_shadow -frames 1000 -app "mesh_pn2 mesh/Suzuran.mesh2 1" -app "albedo_map mesh/SuzuraT.raw 256 256" -app "rotation_axis 0 1 0" -screen "640 640 60" -fsaa 4
./test_shadow -frames 1000 -app "mesh_pn2 mesh/YukiTubaki.mesh2" -app "albedo_map mesh/YukiTuT.raw 256 256" -app "rotation_axis 0 1 0" -screen "640 640 60" -fsaa 4
