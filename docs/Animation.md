# 3D Animation

## File Format Requirements

### Armature

Bones in a tree, each with a rest transformation relative to its parent.

Orientation is quaternion, scale and offset are vec3. Scale is optional (not all animations will use scale, and rest pose necessarily has a scale of 1).

### Actions

An action is a single animation. Ex. "walk", "reload", etc.
Stored as curves for separate components of transforms.
Timeline has any number of keyframes with t measured in seconds.

Max 10 curve "channels" per bone (only present when changed from rest pose):
- Rotation W, X, Y, Z
- Scale X, Y, Z
- Offset X, Y, Z

Curves define interpolation between keyframes:
- Constant (no interpolation)
- Linear (2 end points)
- Cubic Bezier (2 end points, 2 control points)
- Hermite (2 end points, 2 tangents)

### Weights

Each vertex has weights associated with each bone. Most weights will be 0 as bones are generally fairly local. Weights are passed into the shader with associated bone ids for each vertex. We can define an upper limit of bone associations per vertex such that we don't calculate the transforms for every bone on every vertex, only the N most heavily-weighted ones.

### Pipeline

- Each bone has a rest transform relative to its parent: $R_b$
- For each bone, calculate inverse of total transform from model space to bone space in rest pose. We'll call this $R^{-1}_b$
- Apply animation transforms to each bone starting at the root, recursively applying to children. We'll call this $T_b$. 