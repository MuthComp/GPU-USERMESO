/* ----------------------------------------------------------------------
   LAMMPS - Large-scale Atomic/Molecular Massively Parallel Simulator
   http://lammps.sandia.gov, Sandia National Laboratories
   Steve Plimpton, sjplimp@sandia.gov

   Copyright (2003) Sandia Corporation.  Under the terms of Contract
   DE-AC04-94AL85000 with Sandia Corporation, the U.S. Government retains
   certain rights in this software.  This software is distributed under
   the GNU General Public License.

   See the README file in the top-level LAMMPS directory.
------------------------------------------------------------------------- */

#include "stdlib.h"
#include "atom_vec.h"
#include "atom.h"
#include "force.h"
#include "domain.h"
#include "error.h"

using namespace LAMMPS_NS;

/* ---------------------------------------------------------------------- */

AtomVec::AtomVec(LAMMPS *lmp) : Pointers(lmp)
{
  nmax = 0;
  bonds_allow = angles_allow = dihedrals_allow = impropers_allow = 0;
  mass_type = dipole_type = 0;
  size_data_bonus = 0;
  cudable = false;
}

/* ----------------------------------------------------------------------
   no additional args by default
------------------------------------------------------------------------- */

void AtomVec::settings(int narg, char **arg)
{
  //if (narg) error->all(FLERR,"Invalid atom_style command");					// have to edit this line for tdpd to specify the num of species. Ansel

}

/* ----------------------------------------------------------------------
   copy of velocity remap settings from Domain
------------------------------------------------------------------------- */

void AtomVec::init()
{
  deform_vremap = domain->deform_vremap;
  deform_groupbit = domain->deform_groupbit;
  h_rate = domain->h_rate;

  if (lmp->cuda != NULL && cudable == false)
    error->all(FLERR,"USER-CUDA package requires a cuda enabled atom_style");
}

/* ----------------------------------------------------------------------
   unpack one line from Velocities section of data file
------------------------------------------------------------------------- */

void AtomVec::data_vel(int m, char **values)
{
  double **v = atom->v;
  v[m][0] = atof(values[0]);
  v[m][1] = atof(values[1]);
  v[m][2] = atof(values[2]);
}

/* ----------------------------------------------------------------------
   pack velocity info for data file
------------------------------------------------------------------------- */

void AtomVec::pack_vel(double **buf)
{
  double **v = atom->v;
  int *tag = atom->tag;
  int nlocal = atom->nlocal;

  for (int i = 0; i < nlocal; i++) {
    buf[i][0] = tag[i];
    buf[i][1] = v[i][0];
    buf[i][2] = v[i][1];
    buf[i][3] = v[i][2];
  }
}

/* ----------------------------------------------------------------------
   write velocity info to data file
------------------------------------------------------------------------- */

void AtomVec::write_vel(FILE *fp, int n, double **buf)
{
  for (int i = 0; i < n; i++)
    fprintf(fp,"%d %-1.16e %-1.16e %-1.16e\n",
            (int) buf[i][0],buf[i][1],buf[i][2],buf[i][3]);
}

/* ----------------------------------------------------------------------
   pack bond info for data file into buf if non-NULL
   return count of bonds from this proc
   do not count/pack bonds with bondtype = 0
   if bondtype is negative, flip back to positive
------------------------------------------------------------------------- */

int AtomVec::pack_bond(int **buf)
{
  int *tag = atom->tag;
  int *num_bond = atom->num_bond;
  int **bond_type = atom->bond_type;
  int **bond_atom = atom->bond_atom;
  int nlocal = atom->nlocal;
  int newton_bond = force->newton_bond;

  int i,j;
  int m = 0;
  if (newton_bond) {
    for (i = 0; i < nlocal; i++)
      for (j = 0; j < num_bond[i]; j++) {
        if (bond_type[i][j] == 0) continue;
        if (buf) {
          buf[m][0] = MAX(bond_type[i][j],-bond_type[i][j]);
          buf[m][1] = tag[i];
          buf[m][2] = bond_atom[i][j];
        }
        m++;
      }
  } else {
    for (i = 0; i < nlocal; i++)
      for (j = 0; j < num_bond[i]; j++)
        if (tag[i] < bond_atom[i][j]) {
          if (bond_type[i][j] == 0) continue;
          if (buf) {
            buf[m][0] = MAX(bond_type[i][j],-bond_type[i][j]);
            buf[m][1] = tag[i];
            buf[m][2] = bond_atom[i][j];
          }
          m++;
        }
  }

  return m;
}

/* ----------------------------------------------------------------------
   write bond info to data file
------------------------------------------------------------------------- */

void AtomVec::write_bond(FILE *fp, int n, int **buf, int index)
{
  for (int i = 0; i < n; i++) {
    fprintf(fp,"%d %d %d %d\n",index,buf[i][0],buf[i][1],buf[i][2]);
    index++;
  }
}

/* ----------------------------------------------------------------------
   pack angle info for data file into buf if non-NULL
   return count of angles from this proc
   do not count/pack angles with angletype = 0
   if angletype is negative, flip back to positive
------------------------------------------------------------------------- */

int AtomVec::pack_angle(int **buf)
{
  int *tag = atom->tag;
  int *num_angle = atom->num_angle;
  int **angle_type = atom->angle_type;
  int **angle_atom1 = atom->angle_atom1;
  int **angle_atom2 = atom->angle_atom2;
  int **angle_atom3 = atom->angle_atom3;
  int nlocal = atom->nlocal;
  int newton_bond = force->newton_bond;

  int i,j;
  int m = 0;
  if (newton_bond) {
    for (i = 0; i < nlocal; i++)
      for (j = 0; j < num_angle[i]; j++) {
        if (angle_type[i][j] == 0) continue;
        if (buf) {
          buf[m][0] = MAX(angle_type[i][j],-angle_type[i][j]);
          buf[m][1] = angle_atom1[i][j];
          buf[m][2] = angle_atom2[i][j];
          buf[m][3] = angle_atom3[i][j];
        }
        m++;
      }
  } else {
    for (i = 0; i < nlocal; i++)
      for (j = 0; j < num_angle[i]; j++)
        if (tag[i] == angle_atom2[i][j]) {
          if (angle_type[i][j] == 0) continue;
          if (buf) {
            buf[m][0] = MAX(angle_type[i][j],-angle_type[i][j]);
            buf[m][1] = angle_atom1[i][j];
            buf[m][2] = angle_atom2[i][j];
            buf[m][3] = angle_atom3[i][j];
          }
          m++;
        }
  }

  return m;
}

/* ----------------------------------------------------------------------
   write angle info to data file
------------------------------------------------------------------------- */

void AtomVec::write_angle(FILE *fp, int n, int **buf, int index)
{
  for (int i = 0; i < n; i++) {
    fprintf(fp,"%d %d %d %d %d\n",index,
            buf[i][0],buf[i][1],buf[i][2],buf[i][3]);
    index++;
  }
}

/* ----------------------------------------------------------------------
   pack dihedral info for data file
------------------------------------------------------------------------- */

void AtomVec::pack_dihedral(int **buf)
{
  int *tag = atom->tag;
  int *num_dihedral = atom->num_dihedral;
  int **dihedral_type = atom->dihedral_type;
  int **dihedral_atom1 = atom->dihedral_atom1;
  int **dihedral_atom2 = atom->dihedral_atom2;
  int **dihedral_atom3 = atom->dihedral_atom3;
  int **dihedral_atom4 = atom->dihedral_atom4;
  int nlocal = atom->nlocal;
  int newton_bond = force->newton_bond;

  int i,j;
  int m = 0;
  if (newton_bond) {
    for (i = 0; i < nlocal; i++)
      for (j = 0; j < num_dihedral[i]; j++) {
        buf[m][0] = dihedral_type[i][j];
        buf[m][1] = dihedral_atom1[i][j];
        buf[m][2] = dihedral_atom2[i][j];
        buf[m][3] = dihedral_atom3[i][j];
        buf[m][4] = dihedral_atom4[i][j];
        m++;
      }
  } else {
    for (i = 0; i < nlocal; i++)
      for (j = 0; j < num_dihedral[i]; j++)
        if (tag[i] == dihedral_atom2[i][j]) {
          buf[m][0] = dihedral_type[i][j];
          buf[m][1] = dihedral_atom1[i][j];
          buf[m][2] = dihedral_atom2[i][j];
          buf[m][3] = dihedral_atom3[i][j];
          buf[m][4] = dihedral_atom4[i][j];
          m++;
        }
  }
}

/* ----------------------------------------------------------------------
   write dihedral info to data file
------------------------------------------------------------------------- */

void AtomVec::write_dihedral(FILE *fp, int n, int **buf, int index)
{
  for (int i = 0; i < n; i++) {
    fprintf(fp,"%d %d %d %d %d %d\n",index,
            buf[i][0],buf[i][1],buf[i][2],buf[i][3],buf[i][4]);
    index++;
  }
}

/* ----------------------------------------------------------------------
   pack improper info for data file
------------------------------------------------------------------------- */

void AtomVec::pack_improper(int **buf)
{
  int *tag = atom->tag;
  int *num_improper = atom->num_improper;
  int **improper_type = atom->improper_type;
  int **improper_atom1 = atom->improper_atom1;
  int **improper_atom2 = atom->improper_atom2;
  int **improper_atom3 = atom->improper_atom3;
  int **improper_atom4 = atom->improper_atom4;
  int nlocal = atom->nlocal;
  int newton_bond = force->newton_bond;

  int i,j;
  int m = 0;
  if (newton_bond) {
    for (i = 0; i < nlocal; i++)
      for (j = 0; j < num_improper[i]; j++) {
        buf[m][0] = improper_type[i][j];
        buf[m][1] = improper_atom1[i][j];
        buf[m][2] = improper_atom2[i][j];
        buf[m][3] = improper_atom3[i][j];
        buf[m][4] = improper_atom4[i][j];
        m++;
      }
  } else {
    for (i = 0; i < nlocal; i++)
      for (j = 0; j < num_improper[i]; j++)
        if (tag[i] == improper_atom2[i][j]) {
          buf[m][0] = improper_type[i][j];
          buf[m][1] = improper_atom1[i][j];
          buf[m][2] = improper_atom2[i][j];
          buf[m][3] = improper_atom3[i][j];
          buf[m][4] = improper_atom4[i][j];
          m++;
        }
  }
}

/* ----------------------------------------------------------------------
   write improper info to data file
------------------------------------------------------------------------- */

void AtomVec::write_improper(FILE *fp, int n, int **buf, int index)
{
  for (int i = 0; i < n; i++) {
    fprintf(fp,"%d %d %d %d %d %d\n",index,
            buf[i][0],buf[i][1],buf[i][2],buf[i][3],buf[i][4]);
    index++;
  }
}
