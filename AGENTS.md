The filenames must be PascalCase. The functions, classes, and structs must be PascalCase. Variable names must be camelCase. Constants must be CAPITAL_CASE.

# Components:
Screen -> Screen will be the page in browser which will be identified uniquely for example /login and in native it will be also one screen which show in the window like login screen. The user can create multiple screens and even at once they can open multiple screens like how the paint tool has tools screen, main screen etc at the same time. The screen will have also the same layout options, we can grow it, we can align it and so on.
Board -> The board will not be containing anything by default it will be just a plain board. user can put anything inside it. It will be like a canvas. It will be having all other components and then user can connect any board with any screen to display things. It will be the main component for layout.
✅ - Container (clipped scroll region; layout stack, flex, grid, or position)
✅ - Text (This component is going to support the rich text with multiple font sizes at same text and multiple colors. Options like wrap, left, center, right etc. Basically, all the features which text supports must come in this component and user must be able to tweak and play with the component using XML code.)
✅ - Heading (1 to 6; derives from Text with semantic size/weight defaults)
✅ - Paragraph (derives from Text with body copy defaults)
✅ - Button (Apple-style; label sizes the control; single-line text, no wrap)
✅ - Input (Apple-style; filled / bordered / plain; focus ring; placeholder; caret typing)
✅ - Scrollbar
Dropdown
Icons -> The icons will be basically SVGs which user can add by <SVGName /> which will load from the Assets/RoboCountry.pack and RoboIcons.pack.
Menu
Context Menu
SVGs -> SVGs will be directly supported in XML user can write <svg><path> etc</svg> they can write the SVg directly in XML how they write the components. They can use the icons too as <IconName />
Ellipse/Circle
Rectangle
Polygon
Star
✅ - Flexbox
✅ - Grid
✅ - Position
Checkbox
Radio
Toggle
Slider
List
Table


Use the current which already exists as folders to make everything and put everything correctly in these structure. I want to build first for the web so build it for the web platform first but the implementation must be independant of everything so we will just need to put implementation in RoboPlatforms and must not need any other changes to the code. Use the WebAssembly for web and on request of the page the HTMl will be provided too which will be made of the XML so we can get the best SEO.

Programmer writes XML. The compiler turns that XML into C++. That C++ is compiled for the platform. There is no XML interpreter and no IDE. Demo.xml is the page. Run it in the browser with RunDemo.ps1.