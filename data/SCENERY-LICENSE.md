# Map data attribution

`monza-scenery.json` and `monza-gravel.json` contain adapted data from © OpenStreetMap contributors,
available under the Open Data Commons Open Database License (ODbL) 1.0.

- https://www.openstreetmap.org/copyright
- https://opendatacommons.org/licenses/odbl/1-0/

The extracted feature coordinates, source coordinates, anchors and provenance
are supplied in the JSON. Their generated native tables are in
`include/apex3d/generated/monza_world.hpp`. The scene is a Produced Work using
that data and displays OpenStreetMap attribution.

This notice applies to the map database material, not to team artwork or the
project's independently written code.

# Survey data attribution

`monza-track-surface.json` (track edges) and `monza-terrain.json` (ground heights)
contain data adapted from Regione Lombardia, available under the Creative Commons
Attribution 4.0 International licence (CC BY 4.0):

- Database Topografico (DBGT), comune di Monza: "attrezzatura sportiva" polygons of
  type '08' (pista).
- DTM 5x5 - Modello digitale del terreno (ed. 2015), metadata
  r_lombar:dfc98d60-5f02-4a2b-8113-5ad24cc53a9c, sampled every 16 m.

Source: Geoportale della Lombardia, https://www.geoportale.regione.lombardia.it/ ;
licence: https://creativecommons.org/licenses/by/4.0/ . Changes: the track polygons
were turned into edge distances from the measured line and the terrain model was
resampled onto the scene grid, as described in each file and in
`docs/real-track-geometry.md`. The scene displays this attribution.
