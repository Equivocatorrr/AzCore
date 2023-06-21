# Procedural Generation
These are just some notes for how one might go about generating things using a random seed. 

Included are observations about the world around us, ideas for organization, known methods and their pros and cons, and anything else I deem noteworthy.

# Thoughts & Speculation

The world around us is incredibly complicated and detailed. Capturing even a small fraction of that complexity is a seemingly insurmountable task. In order to make this task feasible, we must accept numerous oversimplifications. These oversimplifications are necessary for many reasons, including the limitations of computing, and the actual purpose of generation generally being to be convincing enough within the medium. In the digital era all mediums eventually meet the greatest limiting factor: human perception. Most of the time, especially in artistic mediums, we're happy to settle (and often aim) for being wholly distinct from the real world.

# Methods

The following two methods aren't discrete, but more like sides of a spectrum, since each will require some of both to get meaningful results.

## Top-down
For all intents and purposes, taking a top-down approach is generally the most productive. Starting with perception and working our way down into details as they become necessary or desired can bring quick and well-defined results.

This is the ideal method for artistic types who like to express their ideas as a whole and iteratively increase detail.

## Bottom-up
In contrast, a bottom-up approach requires you to choose a level of detail at the start and has the main drawback being that approaching any top-level goal requires a deep understanding of the systems that brought about that top-level structure in the first place. The main benefit is that this approach can result in generation of new and unexpected behavior in ways a top-down approach never could.

This method is best suited for tinkerers who like to work with complex systems and don't mind unexpected results.

---
Personally, I lean towards bottom-up approaches since they're the most interesting to me. However, how you go about it depends more on the purpose than anything. It may be possible to start with a bottom-up approach at first, but in order to deliver a usable product, one may need to shift into a top-down approach in order to tame the system enough to be useful.

# Hierarchy

Humans like to categorize things hierarchically because it helps us reason about the world as a whole. Hierarchical thinking allows us to make statements with reasonable certainty about extremely complex systems. This tendency has helped us to understand the world around us in ways we simply couldn't otherwise.

This hierarchical view isn't without its problems though, as one form of classification may disagree with another. Take our classifications of fruits and vegetables. We intuitively understand what a fruit is: they're sweet and delicious, pairing well with sweets of all kinds. We also intuitively understand what a vegetable is: generally less sweet, pairing well with savory dishes of all kinds. Now enter the tomato. We've all heard the shocking truth: tomatoes are fruits, but this doesn't work with our intuitive understanding. You wouldn't put tomatoes into sweets, so it must be a vegetable, right?

[In the culinary world, tomatoes are vegetables. This is true. In the world of botany, tomatoes are fruits. This is also true.](https://www.britannica.com/plant/tomato) These are two different systems of classification that happen to use the same terms differently. Both are correct, because they serve their purposes well.

In reality, classification is merely writing down observations that serve a purpose. These being hierarchical helps us reason about things and make predictions about incredibly complicated systems without having to understand everything about the system at once.

## Using Hierarchy and Classification for Proc. Gen.

Since there can be multiple overlapping hierarchies, it can be tempting to look for the most "objective" one to model. This strategy could work, but it doesn't guarantee that the results will look anything like what you want. Instead, I suggest looking for whichever system describes the exact kinds of variation you care about. Whether it makes sense scientifically is secondary to serving its purpose. That is, unless you're going for scientific accuracy.

# Tools

The most basic building block of Proc. Gen. is noise.  
While it's possible to do with traditional pseudo-random number generators (PRNGs), the limitation of only being able to output numbers in a sequence means that generation of discrete things depends on the order in which you generate them.  
Noise, on the other hand, has the benefit of being able to "scrub" anywhere at any time, as well as coming in coherent multi-dimensional variants. Most noise-based generation tends to layer multiple scales to model rough, curved surfaces, but noise can also be used to replace a PRNG for more discrete choices as well. In this way, one could think of noise as a hash where the index is the input and the desired value is the output. It's the best of both worlds!  
At their core, noise functions are pure functions. In other words, they carry no state, and you always get the same output for a single input.

## AzCore Noise

In AzCore, the noise functions all use a series of 64-bit hash functions. However many inputs to that hash function there are, the resulting hashes will have values between 0 and 2<sup>64</sup> (18.447x10<sup>18</sup>).
The exact sequence of values varies based on how many inputs you have (how many multiples of 64 bits).
Many noise implementations are optimized for absolute speed at the cost of a fairly limited domain. This is fine for realtime graphics applications since the domain in those cases are usually quite limited anyway. AzCore Noise prioritizes a larger domain over absolute speed. There are plans to make SIMD variations of these functions, but for now they're scalar.
The larger domain means large-scale procedural generation should have approximately zero repetitions (pending more thorough testing, but the hash functions appear strong in noise_test).

### Types of Noise

#### Integer White Noise
For integer noise, you can just use the hash functions directly.
```C++
u64 hash(u64 x);
u64 hash(u64 x, u64 y);
u64 hash(u64 x, u64 y, u64 z);
u64 hash(u64 x, u64 y, u64 z, u64 w);
```

The rest of the noise types output floating point numbers in the range 0.0 to 1.0.

#### White Noise

The white noise functions take integer coordinates (`u64, vec2i, vec3i, vec4i`) in 1-4 dimensions, and a `u64` seed.

#### Value Noise

The rest of the noise types work with `f64` and `vec2d` inputs and have smooth outputs. There are plans for higher-dimensional noise implementations, but those haven't been done yet.

Note that generating images with these pure functions is far from optimal as the white noise generator is called for every sample between whole numbers, multiplying the hashing work significantly. This is generally why the noise hashing function is hyper-optimized for GPU-based algorithms. A more optimal image generator would generate all of the samples between whole numbers while calling the generators once.

- Linear Noise linearly interpolates between 2<sup>n</sup> points where n is the dimension.
- Cosine Noise uses a cosine interpolator.
- Cubic Noise uses a cubic function to interpolate using 4<sup>n</sup> samples. This one is probably useless ¯\\\_(ツ)_/¯

#### More clever types of noise, courtesy of Ken Perlin

- 1D Perlin Noise uses white noise to generate 2 tangents, which it uses to create a spline between the whole numbers. The value at every whole number is 0.5.
- 2D Perlin Noise uses white noise to generate 4 unit vectors at the corners of the unit square, takes the dot product between those vectors and the sampled point relative to each respective corner of the square, and interpolates the results of the dot products. The value at each corner is 0.5.
- Simplex Noise uses a concept similar to Perlin Noise, but instead of using 2<sup>n</sup> points on an n-dimensional square, it uses n+1 points of simplices. The whole numbers are formed by skewing the lattice of simplices into a lattice of right triangles. n+1 unit vectors are generated, dot products are taken to form "wavelets" whose contributions fall off with the 4th power of euclidean distance, and added together. This scales much more nicely into higher dimensions.

#### Cell Noise

- Voronoi Noise generates random points in a lattice and uses information about the nearest point, such as the distance from the sample, or a random value associated with that point, or a combination of the two.

