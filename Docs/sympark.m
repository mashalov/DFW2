syms Lad Lrc Lfd L1d
detd = (Lad + Lrc)^2 - (Lad + Lrc + Lfd)*(Lad+Lrc+L1d);
nom = Lad^2*(Lfd+L1d) + detd * Lad
simplify(expand(detd))
simplify(expand(nom))


