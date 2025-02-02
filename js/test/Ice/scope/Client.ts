// Copyright (c) ZeroC, Inc.

import { Ice } from "ice";
import { Test, Inner } from "./Test.js";
import { TestHelper } from "../../Common/TestHelper.js";

const test = TestHelper.test;

export class Client extends TestHelper {
    async allTests() {
        const out = this.getWriter();
        const communicator = this.communicator();

        out.write("test using same type name in different Slice modules... ");

        {
            const i1 = new Test.IPrx(communicator, `i1:${this.getTestEndpoint()}`);
            const s1 = new Test.S(0);

            const [s2, s3] = await i1.opS(s1);
            test(s2.equals(s1));
            test(s3.equals(s1));

            const [sseq2, sseq3] = await i1.opSSeq([s1]);
            test(sseq2[0].equals(s1));
            test(sseq3[0].equals(s1));

            const smap1 = new Map([["a", s1]]);
            const [smap2, smap3] = await i1.opSMap(smap1);
            test(smap2.get("a").equals(s1));
            test(smap3.get("a").equals(s1));

            const c1 = new Test.C(s1);

            const [c2, c3] = await i1.opC(c1);
            test(c2.s.equals(s1));
            test(c3.s.equals(s1));

            const [cseq2, cseq3] = await i1.opCSeq([c1]);
            test(cseq2[0].s.equals(s1));
            test(cseq3[0].s.equals(s1));

            const cmap1 = new Map([["a", c1]]);
            const [cmap2, cmap3] = await i1.opCMap(cmap1);
            test(cmap2.get("a").s.equals(s1));
            test(cmap3.get("a").s.equals(s1));

            const e = await i1.opE1(Test.E1.v1);
            test(e == Test.E1.v1);

            const s = await i1.opS1(new Test.S1("S1"));
            test(s.s == "S1");

            const c = await i1.opC1(new Test.C1("C1"));
            test(c.s == "C1");
        }

        {
            const i2 = new Test.Inner.Inner2.IPrx(communicator, `i2:${this.getTestEndpoint()}`);
            const s1 = new Test.Inner.Inner2.S(0);

            const [s2, s3] = await i2.opS(s1);
            test(s2.equals(s1));
            test(s3.equals(s1));

            const [sseq2, sseq3] = await i2.opSSeq([s1]);
            test(sseq2[0].equals(s1));
            test(sseq3[0].equals(s1));

            const smap1 = new Map([["a", s1]]);
            const [smap2, smap3] = await i2.opSMap(smap1);
            test(smap2.get("a").equals(s1));
            test(smap3.get("a").equals(s1));

            const c1 = new Test.Inner.Inner2.C(s1);

            const [c2, c3] = await i2.opC(c1);
            test(c2.s.equals(s1));
            test(c3.s.equals(s1));

            const [cseq2, cseq3] = await i2.opCSeq([c1]);
            test(cseq2[0].s.equals(s1));
            test(cseq3[0].s.equals(s1));

            const cmap1 = new Map([["a", c1]]);
            const [cmap2, cmap3] = await i2.opCMap(cmap1);
            test(cmap2.get("a").s.equals(s1));
            test(cmap3.get("a").s.equals(s1));
        }

        {
            const i3 = new Test.Inner.IPrx(communicator, `i3:${this.getTestEndpoint()}`);
            const s1 = new Test.Inner.Inner2.S(0);

            const [s2, s3] = await i3.opS(s1);
            test(s2.equals(s1));
            test(s3.equals(s1));

            const [sseq2, sseq3] = await i3.opSSeq([s1]);
            test(sseq2[0].equals(s1));
            test(sseq3[0].equals(s1));

            const smap1 = new Map([["a", s1]]);
            const [smap2, smap3] = await i3.opSMap(smap1);
            test(smap2.get("a").equals(s1));
            test(smap3.get("a").equals(s1));

            const c1 = new Test.Inner.Inner2.C(s1);

            const [c2, c3] = await i3.opC(c1);
            test(c2.s.equals(s1));
            test(c3.s.equals(s1));

            const [cseq2, cseq3] = await i3.opCSeq([c1]);
            test(cseq2[0].s.equals(s1));
            test(cseq3[0].s.equals(s1));

            const cmap1 = new Map([["a", c1]]);
            const [cmap2, cmap3] = await i3.opCMap(cmap1);
            test(cmap2.get("a").s.equals(s1));
            test(cmap3.get("a").s.equals(s1));
        }

        {
            const i4 = new Inner.Test.Inner2.IPrx(communicator, `i4:${this.getTestEndpoint()}`);
            const s1 = new Test.S(0);

            const [s2, s3] = await i4.opS(s1);
            test(s2.equals(s1));
            test(s3.equals(s1));

            const [sseq2, sseq3] = await i4.opSSeq([s1]);
            test(sseq2[0].equals(s1));
            test(sseq3[0].equals(s1));

            const smap1 = new Map([["a", s1]]);
            const [smap2, smap3] = await i4.opSMap(smap1);
            test(smap2.get("a").equals(s1));
            test(smap3.get("a").equals(s1));

            const c1 = new Test.C(s1);

            const [c2, c3] = await i4.opC(c1);
            test(c2.s.equals(s1));
            test(c3.s.equals(s1));

            const [cseq2, cseq3] = await i4.opCSeq([c1]);
            test(cseq2[0].s.equals(s1));
            test(cseq3[0].s.equals(s1));

            const cmap1 = new Map([["a", c1]]);
            const [cmap2, cmap3] = await i4.opCMap(cmap1);
            test(cmap2.get("a").s.equals(s1));
            test(cmap3.get("a").s.equals(s1));
        }

        {
            const i1 = new Test.IPrx(communicator, `i1:${this.getTestEndpoint()}`);
            await i1.shutdown();
        }

        out.writeLine("ok");
    }

    async run(args: string[]) {
        let communicator: Ice.Communicator | null = null;
        try {
            [communicator] = this.initialize(args);
            await this.allTests();
        } finally {
            if (communicator) {
                await communicator.destroy();
            }
        }
    }
}
