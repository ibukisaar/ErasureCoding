using NUnit.Framework;
using System;

namespace ErasureCoding.Test {
    public class Tests {
        static readonly Random random = new Random();

        [SetUp]
        public void Setup() {
        }

        [Test]
        public void Test1() {
            const int DataStride = 10;
            const int ECStride = 6;
            const int Rows = 1000;

            var data = new byte[DataStride * Rows];
            var ec = new byte[ECStride * Rows];
            random.NextBytes(data);

            ErasureCoding.Encode(data, ec, DataStride, ECStride);

            var indexes = new ErasureCodingIndex[] { (5, 1), (9, 0), (3, 3) };
            var dataWithEC = data.Clone() as byte[];
            foreach (var (dataMissIndex, ecIndex) in indexes) {
                for (int r = 0; r < Rows; r++) {
                    dataWithEC[r * DataStride + dataMissIndex] = ec[r * ECStride + ecIndex];
                }
            }

            ErasureCoding.Decode(dataWithEC, DataStride, indexes);

            Assert.IsTrue(data.AsSpan().SequenceEqual(dataWithEC));
        }
    }
}