#include <assert.h>

#include <string>

#include "flags.hpp"
#include "load.hpp"
#include "sml.hpp"

#define NPHASE 11
#define GET(X, i, j) X[i * NPHASE + j]

extern adios2::ADIOS ad;
adios2::Engine reader;
adios2::IO reader_io;

void load_init(const std::string &filename)
{
    reader_io = ad.DeclareIO("headload");
    reader = reader_io.Open(filename, adios2::Mode::Read);
}

void load_finalize()
{
    reader.Close();
}

adios2::StepStatus load_data(Particles &idiv, Particles &ediv, t_ParticlesList &iesc, t_ParticlesList &eesc)
{
    // Clear vector
    idiv.clear();
    ediv.clear();
    iesc.clear();
    eesc.clear();

    adios2::StepStatus status = reader.BeginStep();
    if (status == adios2::StepStatus::OK)
    {
        // Inquire variables
        auto var_igid = reader_io.InquireVariable<long>("igid");
        auto var_egid = reader_io.InquireVariable<long>("egid");
        auto var_iflag = reader_io.InquireVariable<int>("iflag");
        auto var_eflag = reader_io.InquireVariable<int>("eflag");
        auto var_istep = reader_io.InquireVariable<int>("istep");
        auto var_estep = reader_io.InquireVariable<int>("estep");
        auto var_idw = reader_io.InquireVariable<float>("idw");
        auto var_edw = reader_io.InquireVariable<float>("edw");
        auto var_iphase = reader_io.InquireVariable<float>("iphase");
        auto var_ephase = reader_io.InquireVariable<float>("ephase");

        var_igid.SetSelection({{0}, {var_igid.Shape()[0]}});
        var_egid.SetSelection({{0}, {var_egid.Shape()[0]}});
        var_iflag.SetSelection({{0}, {var_iflag.Shape()[0]}});
        var_eflag.SetSelection({{0}, {var_eflag.Shape()[0]}});
        var_istep.SetSelection({{0}, {var_istep.Shape()[0]}});
        var_estep.SetSelection({{0}, {var_estep.Shape()[0]}});
        var_idw.SetSelection({{0}, {var_idw.Shape()[0]}});
        var_edw.SetSelection({{0}, {var_edw.Shape()[0]}});
        var_iphase.SetSelection({{0, 0}, {var_iphase.Shape()[0], var_iphase.Shape()[1]}});
        var_ephase.SetSelection({{0, 0}, {var_ephase.Shape()[0], var_ephase.Shape()[1]}});

        std::vector<long> igid;
        std::vector<long> egid;
        std::vector<int> iflag;
        std::vector<int> eflag;
        std::vector<int> istep;
        std::vector<int> estep;
        std::vector<float> idw;
        std::vector<float> edw;
        std::vector<float> iphase;
        std::vector<float> ephase;

        reader.Get<long>(var_igid, igid);
        reader.Get<long>(var_egid, egid);
        reader.Get<int>(var_iflag, iflag);
        reader.Get<int>(var_eflag, eflag);
        reader.Get<int>(var_istep, istep);
        reader.Get<int>(var_estep, estep);
        reader.Get<float>(var_idw, idw);
        reader.Get<float>(var_edw, edw);
        reader.Get<float>(var_iphase, iphase);
        reader.Get<float>(var_ephase, ephase);
        reader.EndStep();

        assert(iphase.size() / igid.size() == NPHASE);
        assert(ephase.size() / egid.size() == NPHASE);

        // populate particles
        for (int i = 0; i < igid.size(); i++)
        {
            struct Particle iptl;
            iptl.gid = igid[i];
            iptl.flag = iflag[i];
            iptl.esc_step = istep[i];
            iptl.r = GET(iphase, i, 0);
            iptl.z = GET(iphase, i, 1);
            iptl.phi = GET(iphase, i, 2);
            iptl.rho = GET(iphase, i, 3);
            iptl.w1 = GET(iphase, i, 4);
            iptl.w2 = GET(iphase, i, 5);
            iptl.mu = GET(iphase, i, 6);
            iptl.w0 = GET(iphase, i, 7);
            iptl.f0 = GET(iphase, i, 8);
            iptl.psi= GET(iphase, i, 9);
            iptl.B  = GET(iphase, i,10);
            iptl.dw = idw[i];

            int flag1; // tmp flag
            flag1 = iflag[i];

            Flags fl(flag1); // decode flags

            // save to div or esc
            if (fl.escaped)
            {
                // add to esc
                iesc.insert(std::pair<long long, Particle>(iptl.gid, iptl));
            }
            else
            {
                // add to div
                idiv.push_back(iptl);
            }
        }

        for (int i = 0; i < egid.size(); i++)
        {
            struct Particle eptl;
            eptl.gid = egid[i];
            eptl.flag = eflag[i];
            eptl.esc_step = estep[i];
            eptl.r = GET(ephase, i, 0);
            eptl.z = GET(ephase, i, 1);
            eptl.phi = GET(ephase, i, 2);
            eptl.rho = GET(ephase, i, 3);
            eptl.w1 = GET(ephase, i, 4);
            eptl.w2 = GET(ephase, i, 5);
            eptl.mu = GET(ephase, i, 6);
            eptl.w0 = GET(ephase, i, 7);
            eptl.f0 = GET(ephase, i, 8);
            eptl.psi= GET(ephase, i, 9);
            eptl.B  = GET(ephase, i,10);
            eptl.dw = edw[i];

            int flag1; // tmp flag
            flag1 = eflag[i];

            Flags fl(flag1); // decode flags

            // save to div or esc
            if (fl.escaped)
            {
                // add to esc
                eesc.insert(std::pair<long long, Particle>(eptl.gid, eptl));
            }
            else
            {
                // add to div
                ediv.push_back(eptl);
            }
        }
    }

    return status;
}
