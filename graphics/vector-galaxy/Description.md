# Vector Galaxy — The Shape of Similarity

## The invitation

What does **similarity** look like when an index of hundreds of objects becomes a living universe? Vector Galaxy turns feature-space retrieval into a luminous, explorable topographic nebula. Search vectors move through three dimensions; cyan rings reveal the neighbours actually returned by the managed hnswlib index. The geometry is not a painted backdrop: a triangulated 851-vertex heightfield is analysed with libigl and Eigen, and its normals influence the light of the mesh.

## Real computation

Each of 851 points has a deterministic three-dimensional feature vector. `hnswlib::HierarchicalNSW<float>` is populated once and returns `k` closest candidates for a user-controlled query. Separate Eigen vertex and face matrices are passed to `igl::per_vertex_normals`. The viewer projects the mesh, draws its edges, applies a normal-driven color, and highlights precisely the returned HNSW indices.

This is an educational, deterministic, small-scale ANN demonstration, not a claim that a given HNSW result is an exact globally optimal nearest neighbour or a production artwork embedding model.

## Experience and controls

Use **Search position X/Y/Z** to explore the indexed field, **Nearest stars** to control neighbour count, and **Telescope zoom** to navigate the image. Freeze or resume orbital motion and reset the experiment at any time. Hover descriptions explain which library owns which computation; the renderer does not substitute its own nearest-neighbour algorithm.

The application is based on the existing installed SDL2 + Dear ImGui renderer. It consumes the installed hnswlib, libigl, and Eigen interfaces. The scene contains no downloaded images or untracked external dependencies.

## Verification

Run `vector-galaxy.exe --verify` or the `VectorGalaxyAlgorithm` CTest to check index return cardinality and finite mesh normals. The GUI is a separate, interactive native Windows application. **[not verified]** Native BCC64X compilation and visual acceptance remain pending.
