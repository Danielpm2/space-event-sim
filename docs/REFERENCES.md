# References

Where to find the math and physics behind each simulation. The simulations are visual approximations: several constants are tuned for looks and pacing, and the notes below say where the code is faithful and where it is stylized.

## General background

- C. W. Misner, K. S. Thorne, J. A. Wheeler, *Gravitation* (1973). The standard reference for geodesics and black holes.
- S. M. Carroll, *Spacetime and Geometry* (2004). Lecture-note version: [arXiv:gr-qc/9712019](https://arxiv.org/abs/gr-qc/9712019).
- M. Maggiore, *Gravitational Waves, Volume 1: Theory and Experiments* (Oxford, 2007).
- D. Lorimer, M. Kramer, *Handbook of Pulsar Astronomy* (Cambridge, 2004).

## Black Hole

Code: [src/core/BlackHoleSim.cpp](../src/core/BlackHoleSim.cpp), [shaders/black_hole.frag.glsl](../shaders/black_hole.frag.glsl)

| What the code does | Math | Source |
| --- | --- | --- |
| Light bending (`acc = -1.5 rs h² r̂ / r⁵`) | Photon orbit equation in Schwarzschild spacetime, $\frac{d^2u}{d\varphi^2} + u = \frac{3}{2} r_s u^2$ with $u = 1/r$. The shader integrates the equivalent Cartesian form. | MTW (Schwarzschild geodesics); Carroll ch. 5 |
| Horizon radius | Kerr outer horizon, $r_+ = M\left(1 + \sqrt{1 - a^2}\right)$ | Bardeen, Press, Teukolsky, ApJ 178, 347 (1972) |
| Inner disk edge (ISCO) | Prograde ISCO of a Kerr hole, closed form in `BlackHoleSim::iscoRadius` | Bardeen, Press, Teukolsky (1972) |
| Disk temperature $T \propto r^{-3/4}$ | Thin-disk temperature profile | Shakura & Sunyaev, A&A 24, 337 (1973); Novikov & Thorne (1973) |
| Doppler beaming, $D = 1/\left(\gamma(1 - \beta\cos\theta)\right)$, and gravitational redshift $\sqrt{1 - r_s/r}$ | Relativistic Doppler and redshift factors, combined as $g$; intensity scales roughly as $g^{3.5}$ here | Rybicki & Lightman, *Radiative Processes in Astrophysics*, ch. 4 |
| Overall look | Ray-traced images of a thin disk around a black hole | Luminet, A&A 75, 228 (1979); James et al., [arXiv:1502.03808](https://arxiv.org/abs/1502.03808) (the *Interstellar* renderer) |

Stylized: the rays use the Schwarzschild bending even when spin is non-zero (spin only changes the horizon, ISCO and disk rotation), so frame dragging is not simulated.

## Pulsar

Code: [src/core/PulsarSim.cpp](../src/core/PulsarSim.cpp)

| What the code does | Math | Source |
| --- | --- | --- |
| Lighthouse model: beams along $\pm$ the magnetic axis, rotating with the star | Beam direction $\hat m(t) = (\sin\alpha\cos\Omega t,\ \cos\alpha,\ \sin\alpha\sin\Omega t)$ with tilt $\alpha$ | Gold, Nature 218, 731 (1968); Lorimer & Kramer |
| Pulse brightness for an observer | Gaussian in the angle $\theta$ between observer and beam axis, $\exp(-\theta^2/\sigma^2)$ | Lorimer & Kramer (pulse profiles) |
| Pulsar discovery | First detection of pulsed radio sources | Hewish et al., Nature 217, 709 (1968) |
| Magnetosphere and charged-particle outflow | Rotating magnetized neutron star as a particle accelerator | Goldreich & Julian, ApJ 157, 869 (1969) |

Stylized: the beams are cones with a Gaussian profile, and particles move radially at constant speed instead of following field lines.

## Supernova

Code: [src/core/SupernovaSim.cpp](../src/core/SupernovaSim.cpp), [shaders/supernova.frag.glsl](../shaders/supernova.frag.glsl)

| What the code does | Math | Source |
| --- | --- | --- |
| Core collapse followed by an explosion | Overview of the collapse, bounce and neutrino-driven explosion mechanism | Janka, Ann. Rev. Nucl. Part. Sci. 62, 407 (2012), [arXiv:1206.2503](https://arxiv.org/abs/1206.2503) |
| Expanding shock radius `R ∝ √E · t^0.68` | Self-similar blast wave. The exact Sedov-Taylor solution is $R = \xi\left(E t^2/\rho\right)^{1/5}$, i.e. $R \propto t^{0.4}$. The code uses a larger exponent as a compromise between the early free-expansion phase ($R \propto t$) and the Sedov phase. The energy dependence is also stylized: Sedov gives $R \propto E^{1/5}$, the code uses $\sqrt{E}$. | Taylor, Proc. R. Soc. A 201, 159 (1950); Sedov, *Similarity and Dimensional Methods in Mechanics* (1959); Truelove & McKee, ApJS 120, 299 (1999) |
| Explosion energy scale | About $10^{51}$ erg of kinetic energy (the `Explosion energy` slider is relative to this) | Janka (2012) |

Stylized: shell thickness, clumpiness and brightness decay are artistic; there is no hydrodynamics.

## Neutron Star Merger

Code: [src/core/MergerSim.cpp](../src/core/MergerSim.cpp)

| What the code does | Math | Source |
| --- | --- | --- |
| Orbital frequency | Kepler's third law, $\omega = \sqrt{G M / a^3}$ with $M = m_1 + m_2$ | Any mechanics text |
| Separation shrinks until merger | Orbital decay by gravitational-wave emission, $\dot a = -\frac{64}{5}\frac{G^3 m_1 m_2 (m_1+m_2)}{c^5 a^3}$. The code uses a tuned rate (`kInspiralRate`) to fit about 16 s. | Peters, Phys. Rev. 136, B1224 (1964); Maggiore |
| Ripples at twice the orbital frequency | Quadrupole radiation has frequency $f_{GW} = 2 f_{orb}$ and an amplitude growing as the orbit tightens | Maggiore |
| Curved grid | The well depth uses a softened $-m/r$ potential as a stand-in for an embedding diagram | MTW (Flamm paraboloid, embedding diagrams) |
| Observed merger | First binary neutron star merger with gravitational waves and light | Abbott et al., PRL 119, 161101 (2017), [arXiv:1710.05832](https://arxiv.org/abs/1710.05832) |
| Kilonova ejecta and jets | Radioactive glow of r-process ejecta; short gamma-ray burst jets | Li & Paczynski, ApJ 507, L59 (1998), [arXiv:astro-ph/9807272](https://arxiv.org/abs/astro-ph/9807272); Metzger, Living Rev. Relativ. 23, 1 (2020), [arXiv:1910.01617](https://arxiv.org/abs/1910.01617) |

Stylized: the grid is a Newtonian-looking picture, not a solution of the Einstein equations. The tidal stretch and merger remnant are visual effects.

## Magnetar

Code: [src/core/MagnetarSim.cpp](../src/core/MagnetarSim.cpp)

| What the code does | Math | Source |
| --- | --- | --- |
| Dipole field lines | In polar form a dipole field line is $r = L\sin^2\theta$, where $L$ labels the shell (equatorial crossing). Lines start at $\theta_{min} = \arcsin\sqrt{1/L}$ so they meet the surface at $r = 1$. | Jackson, *Classical Electrodynamics*, ch. 5 |
| Twisted magnetosphere, azimuth shifted by the twist angle | Twisting the field lines stores energy in the magnetosphere | Thompson, Lyutikov, Kulkarni, ApJ 574, 332 (2002), [arXiv:astro-ph/0110677](https://arxiv.org/abs/astro-ph/0110677) |
| Magnetar model, field strength of about $10^{14}$-$10^{15}$ G | Ultra-strong fields power the bursts | Duncan & Thompson, ApJ 392, L9 (1992); Kaspi & Beloborodov, ARA&A 55, 261 (2017), [arXiv:1703.00068](https://arxiv.org/abs/1703.00068) |
| Giant flare after a starquake | Crust fracture releasing magnetic energy; first well-observed example | Hurley et al., Nature 434, 1098 (2005) |

Stylized: the twist and stress build-up are linear, and the flare is a visual effect rather than a magnetohydrodynamic model.

## Rendering

| Topic | Source |
| --- | --- |
| Filmic tone mapping (`composite.frag.glsl`, `menu_bg.frag.glsl`) | K. Narkowicz, [ACES Filmic Tone Mapping Curve](https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/) |
| Milky Way sky map | Solar System Scope, CC BY 4.0, based on NASA/ESA data (see [assets/CREDITS.txt](../assets/CREDITS.txt)) |
