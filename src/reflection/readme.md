# Reflection System

Static and runtime reflection system used for various tasks such as generating Editor displays or adding systems to entities.

Based on [this blog post](https://github.com/preshing/FlexibleReflection/tree/a1c5a518e000383a89aca61116329d6fc09a6b3c).

# TODO
 - Separate the editor stuff from the rest to avoid having to include imgui in every lib.
 - Add some sort of scope utility to separate families of types
 - Wrap some primitives to allow more fluidity in UI widget selection (i.e. wrap glm::vec3 in a Color class to allow using the color picker)
 - Prettify type names when displaying them in Editor