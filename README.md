# ErasureCoding
C# Erasure Coding（纠删码）

## 使用说明
```csharp
public static class ErasureCoding {
  /// <summary>
  /// 填充纠错码
  /// </summary>
  /// <param name="data">数据</param>
  /// <param name="erasureCode">纠错码</param>
  /// <param name="dataStride">数据跨度，大于0且小于255。data长度必须是dataStride的整数倍</param>
  /// <param name="erasureCodeStride">纠错码跨度，大于0且小于255。erasureCode长度必须是erasureCodeStride的整数倍</param>
  public static void Encode(ReadOnlySpan<byte> data, Span<byte> erasureCode, int dataStride, int erasureCodeStride);
  
  /// <summary>
  /// 恢复数据
  /// </summary>
  /// <param name="dataWithEC">数据与纠错码</param>
  /// <param name="stride">原始数据的跨度，大于0且小于255。dataWithEC长度必须是stride的整数倍</param>
  /// <param name="indexes">指定每个跨度缺失的数据索引以及代替的纠错码索引</param>
  public static void Decode(Span<byte> dataWithEC, int stride, ReadOnlySpan<ErasureCodingIndex> indexes);
}
```

假设我们有42字节的数据，每6个字节想使用4个纠错码，共需要28字节的纠错码。
![image](https://github.com/ibukisaar/ErasureCoding/raw/master/imgs/1_1.png)

调用ErasureCoding.Encode方法来填充纠错码，然后将数据如图所示拆成10份存储或传输。
```csharp
var data = new byte[42];
var ec = new byte[28];
random.NextBytes(data);

ErasureCoding.Encode(data, ec, 6, 4);
```

假设第2、4、8份数据丢失，随意取2份纠错码填充到原始数据中（下例取第1、3份纠错码到第2、4份数据中），然后调用ErasureCoding.Decode方法来恢复数据。
![image](https://github.com/ibukisaar/ErasureCoding/raw/master/imgs/1_2.png)
![image](https://github.com/ibukisaar/ErasureCoding/raw/master/imgs/1_3.png)

```csharp
var indexes = new ErasureCodingIndex[] { (1, 0), (3, 2) };
foreach (var (dataMissIndex, ecIndex) in indexes) {
  for (int r = 0; r < 7; r++) {
    data[r * 6 + dataMissIndex] = ec[r * 4 + ecIndex]; // 用纠错码填充数据
  }
}

ErasureCoding.Decode(data, 6, indexes); // 恢复数据
```

## 原理
![image](https://github.com/ibukisaar/ErasureCoding/raw/master/imgs/2_1.png)
![image](https://github.com/ibukisaar/ErasureCoding/raw/master/imgs/2_2.png)
![image](https://github.com/ibukisaar/ErasureCoding/raw/master/imgs/2_3.png)
![image](https://github.com/ibukisaar/ErasureCoding/raw/master/imgs/2_4.png)

上述矩阵中的元素都是GF(2^8)中的元素，其中a^i是GF(2^8)具体的元素。
